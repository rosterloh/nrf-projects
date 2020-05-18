#include <zephyr/types.h>
#include <sys/ring_buffer.h>
#include <drivers/uart.h>
#include <drivers/gpio.h>

#define MODULE uart_handler
#include "module_state_event.h"
#include "cdc_data_event.h"
#include "uart_data_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_UART_LOG_LEVEL);

#define UART_BUF_SIZE CONFIG_BRIDGE_BUF_SIZE

#define UART_SLAB_BLOCK_SIZE sizeof(struct uart_rx_buf)
#define UART_SLAB_BLOCK_COUNT (2 * CONFIG_BRIDGE_UART_BUF_COUNT)
#define UART_SLAB_ALIGNMENT 4
#define UART_RX_TIMEOUT_MS 1

#if (defined(CONFIG_DEVICE_POWER_MANAGEMENT) &&\
	defined(CONFIG_SYS_PM_POLICY_APP))
#define UART_SET_PM_STATE true
#else
#define UART_SET_PM_STATE false
#endif


struct uart_rx_buf {
	atomic_t ref_counter;
	size_t len;
	u8_t buf[UART_BUF_SIZE];
};

struct uart_tx_buf {
	struct ring_buf rb;
	u32_t buf[UART_BUF_SIZE];
};

BUILD_ASSERT((sizeof(struct uart_rx_buf) % UART_SLAB_ALIGNMENT) == 0);

/* Blocks from the same slab is used for RX for all UART instances */
/* TX has inidividual ringbuffers per UART instance */

K_MEM_SLAB_DEFINE(uart_rx_slab, UART_SLAB_BLOCK_SIZE, UART_SLAB_BLOCK_COUNT, UART_SLAB_ALIGNMENT);

static struct device *udev;
static struct device *reset_gpio;
static struct uart_tx_buf uart_tx_ringbuf;
static u32_t uart_default_baudrate;
/* UART RX only enabled when there is one or more subscribers (power saving) */
static int subscriber_count;
static bool enable_rx_retry;
static atomic_t uart_tx_started;

static bool framing_error_msg_sent;
static char framing_error_msg[] =
	"[UART FRAMING ERROR! CHECK BAUDRATE!]";

static void enable_uart_rx();
// static void disable_uart_rx();
static void set_uart_power_state(bool active);
static int uart_tx_start();
static void uart_tx_finish(size_t len);

static inline struct uart_rx_buf *block_start_get(u8_t *buf)
{
	size_t block_num;

	/* blocks are fixed size units from a continuous memory slab: */
	/* round down to the closest unit size to find beginning of block. */

	block_num =
		(((size_t)buf - (size_t)uart_rx_slab.buffer) / UART_SLAB_BLOCK_SIZE);

	return (struct uart_rx_buf *) &uart_rx_slab.buffer[block_num * UART_SLAB_BLOCK_SIZE];
}

static struct uart_rx_buf *uart_rx_buf_alloc(void)
{
	struct uart_rx_buf *buf;
	int err;

	/* Async UART driver returns pointers to received data as */
	/* offsets from beginning of RX buffer block. */
	/* This code uses a reference counter to keep track of the number of */
	/* references within a single RX buffer block */

	err = k_mem_slab_alloc(&uart_rx_slab, (void **) &buf, K_NO_WAIT);
	if (err) {
		return NULL;
	}

	atomic_set(&buf->ref_counter, 1);

	return buf;
}

static void uart_rx_buf_ref(void *buf)
{
	__ASSERT_NO_MSG(buf);

	atomic_inc(&(block_start_get(buf)->ref_counter));
}

static void uart_rx_buf_unref(void *buf)
{
	__ASSERT_NO_MSG(buf);

	struct uart_rx_buf *uart_buf = block_start_get(buf);
	atomic_t ref_counter = atomic_dec(&uart_buf->ref_counter);

	/* ref_counter is the uart_buf->ref_counter value prior to decrement */
	if (ref_counter == 1) {
		k_mem_slab_free(&uart_rx_slab, (void **)&uart_buf);
	}
}

static void uart_callback(struct uart_event *evt, void *user_data)
{
	struct uart_data_event *event;
	struct uart_rx_buf *buf;
	int err;

	switch (evt->type) {
	case UART_RX_RDY:
		uart_rx_buf_ref(evt->data.rx.buf);

		event = new_uart_data_event();
		event->buf = &evt->data.rx.buf[evt->data.rx.offset];
		event->len = evt->data.rx.len;
		EVENT_SUBMIT(event);
		break;
	case UART_RX_BUF_RELEASED:
		if (evt->data.rx_buf.buf) {
			uart_rx_buf_unref(evt->data.rx_buf.buf);
		}
		break;
	case UART_RX_BUF_REQUEST:
		buf = uart_rx_buf_alloc();
		if (buf == NULL) {
			LOG_WRN("UART RX overflow");
			break;
		}

		err = uart_rx_buf_rsp(udev, buf->buf, sizeof(buf->buf));
		if (err) {
			LOG_ERR("uart_rx_buf_rsp: %d", err);
			uart_rx_buf_unref(buf);
		}
		break;
	case UART_RX_DISABLED:
		if (enable_rx_retry) {
			enable_uart_rx();
			enable_rx_retry = false;
		} else if (UART_SET_PM_STATE) {
			set_uart_power_state(false);
		}
		break;
	case UART_TX_DONE:
		uart_tx_finish(evt->data.tx.len);

		if (ring_buf_is_empty(&uart_tx_ringbuf.rb)) {
			atomic_set(&uart_tx_started, false);
		} else {
			uart_tx_start();
		}
		break;
	case UART_TX_ABORTED:
		uart_tx_finish(evt->data.tx.len);
		atomic_set(&uart_tx_started, false);
		break;
	case UART_RX_STOPPED:
		LOG_WRN("UART stop reason %d", evt->data.rx_stop.reason);
		if (evt->data.rx_stop.data.buf) {
			uart_rx_buf_unref(evt->data.rx_stop.data.buf);
		}

		if (evt->data.rx_stop.reason & UART_ERROR_FRAMING &&
			!framing_error_msg_sent) {
			/* Send error message so user can identify baud rate issues */
			event = new_uart_data_event();
			event->buf = framing_error_msg;
			event->len = sizeof(framing_error_msg);
			EVENT_SUBMIT(event);

			framing_error_msg_sent = true;
		}

		if ((evt->data.rx_stop.reason & UART_ERROR_FRAMING) == 0) {
			/* Unexpected stop: restart RX after DISABLED event */
			/* Avoid restarting upon framing error, as this is unlikely to */
			/* recover until UART is reopened with proper baud rate */
			enable_rx_retry = true;
		}
		break;
	default:
		LOG_ERR("Unexpected event: %d", evt->type);
		__ASSERT_NO_MSG(false);
		break;
	}
}
/*
static void set_uart_baudrate(u32_t baudrate)
{
	struct uart_config cfg;
	int err;

	if (baudrate == 0) {
		return;
	}

	err = uart_config_get(udev, &cfg);
	if (err) {
		LOG_ERR("uart_config_get: %d", err);
		return;
	}

	if (cfg.baudrate == baudrate) {
		return;
	}

	cfg.baudrate = baudrate;

	err = uart_configure(udev, &cfg);
	if (err) {
		LOG_ERR("uart_configure: %d", err);
		return;
	}
}
*/
static void set_uart_power_state(bool active)
{
#if UART_SET_PM_STATE
	int err;
	u32_t current_state;
	u32_t target_state;

	target_state = active ? DEVICE_PM_ACTIVE_STATE : DEVICE_PM_SUSPEND_STATE;

	err = device_get_power_state(udev, &current_state);
	if (err) {
		LOG_ERR("device_get_power_state: %d", err);
		return;
	}

	if (current_state == target_state) {
		return;
	}

	err = device_set_power_state(
		udev,
		target_state,
		NULL,
		NULL);
	if (err) {
		LOG_ERR("device_set_power_state: %d", err);
		return;
	}
#endif
}

static void enable_uart_rx(void)
{
	int err;
	struct uart_rx_buf *buf;

	err = uart_callback_set(udev, uart_callback, NULL);
	if (err) {
		LOG_ERR("uart_callback_set: %d", err);
		return;
	}

	buf = uart_rx_buf_alloc();
	if (!buf) {
		LOG_ERR("uart_rx_buf_alloc error");
		return;
	}

	err = uart_rx_enable(udev, buf->buf, sizeof(buf->buf), UART_RX_TIMEOUT_MS);
	if (err) {
		uart_rx_buf_unref(buf);
		LOG_ERR("uart_rx_enable: %d", err);
		return;
	}
}
/*
static void disable_uart_rx(void)
{
	int err;

	err = uart_rx_disable(udev);
	if (err) {
		LOG_ERR("uart_rx_disable: %d", err);
		return;
	}
}
*/
static int uart_tx_start(void)
{
	int len;
	int err;
	u8_t *buf;

	len = ring_buf_get_claim(
			&uart_tx_ringbuf.rb,
			&buf,
			sizeof(uart_tx_ringbuf.buf));

	err = uart_tx(udev, buf, len, 0);
	if (err) {
		LOG_ERR("uart_tx: %d", err);
		uart_tx_finish(0);
		return err;
	}

	return 0;
}

static void uart_tx_finish(size_t len)
{
	int err;

	err = ring_buf_get_finish(&uart_tx_ringbuf.rb, len);
	if (err) {
		LOG_ERR("ring_buf_get_finish: %d", err);
	}
}

static int uart_tx_enqueue(u8_t *data, size_t data_len)
{
	atomic_t started;
	u32_t written;
	int err;

	written = ring_buf_put(&uart_tx_ringbuf.rb, data, data_len);
	if (written == 0) {
		return -ENOMEM;
	}

	started = atomic_set(&uart_tx_started, true);
	if (!started) {
		err = uart_tx_start();
		if (err) {
			LOG_ERR("uart_tx_start: %d", err);
			atomic_set(&uart_tx_started, false);
		}
	}

	if (written == data_len) {
		return 0;
	} else {
		return -ENOMEM;
	}

	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	int err;

	if (is_uart_data_event(eh)) {
		const struct uart_data_event *event =
			cast_uart_data_event(eh);

		/* All subscribers have gotten a chance to copy data at this point */
		uart_rx_buf_unref(event->buf);

		return true;
	}

	if (is_cdc_data_event(eh)) {
		const struct cdc_data_event *event =
			cast_cdc_data_event(eh);

		err = uart_tx_enqueue(event->buf, event->len);
		if (err == -ENOMEM) {
			LOG_WRN("CDC->UART overflow");
		} else if (err) {
			LOG_ERR("uart_tx_enqueue: %d", err);
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			struct uart_config cfg;

			udev = device_get_binding(DT_LABEL(DT_NODELABEL(uart1)));
			if (!udev) {
				LOG_ERR("%s not available", DT_LABEL(DT_NODELABEL(uart1)));
				return false;
			}

			err = uart_config_get(udev, &cfg);
			if (err) {
				LOG_ERR("uart_config_get: %d", err);
				return false;
			}
			uart_default_baudrate = cfg.baudrate;
			subscriber_count = 0;
			enable_rx_retry = false;

			atomic_set(&uart_tx_started, false);

			ring_buf_init(
				&uart_tx_ringbuf.rb,
				sizeof(uart_tx_ringbuf.buf),
				uart_tx_ringbuf.buf);

			if (UART_SET_PM_STATE) {
				set_uart_power_state(false);
			}

			reset_gpio = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));
			if (reset_gpio < 0) {
				LOG_ERR("%s not available",
					DT_LABEL(DT_NODELABEL(gpio0)));
			}

			err = gpio_pin_configure(reset_gpio, 24,
							GPIO_OUTPUT | GPIO_OPEN_DRAIN);
			if (err) {
				LOG_ERR("Failed to configure WIFI_EN. %d", err);
			}
			err = gpio_pin_configure(reset_gpio, 16, GPIO_OUTPUT);
			if (err) {
				LOG_ERR("Failed to configure BOOT_MODE. %d", err);
			}

			gpio_pin_set(reset_gpio, 16, 1);
			k_msleep(100);
			gpio_pin_set(reset_gpio, 24, 0);
			k_msleep(100);
			gpio_pin_set(reset_gpio, 24, 1);
			k_msleep(100);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, cdc_data_event);
EVENT_SUBSCRIBE_FINAL(MODULE, uart_data_event);
