#ifndef ZEPHYR_DRIVERS_SENSOR_SEESAW_SEESAW_H_
#define ZEPHYR_DRIVERS_SENSOR_SEESAW_SEESAW_H_

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>

/** The module base addresses for different seesaw modules. */
enum {
        SEESAW_STATUS_BASE = 0x00,
        SEESAW_GPIO_BASE = 0x01,
        SEESAW_SERCOM0_BASE = 0x02,

        SEESAW_TIMER_BASE = 0x08,
        SEESAW_ADC_BASE = 0x09,
        SEESAW_DAC_BASE = 0x0A,
        SEESAW_INTERRUPT_BASE = 0x0B,
        SEESAW_DAP_BASE = 0x0C,
        SEESAW_EEPROM_BASE = 0x0D,
        SEESAW_NEOPIXEL_BASE = 0x0E,
        SEESAW_TOUCH_BASE = 0x0F,
        SEESAW_KEYPAD_BASE = 0x10,
        SEESAW_ENCODER_BASE = 0x11,
};

/** GPIO module function addres registers */
enum {
        SEESAW_GPIO_DIRSET_BULK = 0x02,
        SEESAW_GPIO_DIRCLR_BULK = 0x03,
        SEESAW_GPIO_BULK = 0x04,
        SEESAW_GPIO_BULK_SET = 0x05,
        SEESAW_GPIO_BULK_CLR = 0x06,
        SEESAW_GPIO_BULK_TOGGLE = 0x07,
        SEESAW_GPIO_INTENSET = 0x08,
        SEESAW_GPIO_INTENCLR = 0x09,
        SEESAW_GPIO_INTFLAG = 0x0A,
        SEESAW_GPIO_PULLENSET = 0x0B,
        SEESAW_GPIO_PULLENCLR = 0x0C,
};

/** status module function addres registers */
enum {
        SEESAW_STATUS_HW_ID = 0x01,
        SEESAW_STATUS_VERSION = 0x02,
        SEESAW_STATUS_OPTIONS = 0x03,
        SEESAW_STATUS_TEMP = 0x04,
        SEESAW_STATUS_SWRST = 0x7F,
};

/** timer module function addres registers */
enum {
        SEESAW_TIMER_STATUS = 0x00,
        SEESAW_TIMER_PWM = 0x01,
        SEESAW_TIMER_FREQ = 0x02,
};
	
/** ADC module function addres registers */
enum {
        SEESAW_ADC_STATUS = 0x00,
        SEESAW_ADC_INTEN = 0x02,
        SEESAW_ADC_INTENCLR = 0x03,
        SEESAW_ADC_WINMODE = 0x04,
        SEESAW_ADC_WINTHRESH = 0x05,
        SEESAW_ADC_CHANNEL_OFFSET = 0x07,
};

/** Sercom module function addres registers */
enum {
        SEESAW_SERCOM_STATUS = 0x00,
        SEESAW_SERCOM_INTEN = 0x02,
        SEESAW_SERCOM_INTENCLR = 0x03,
        SEESAW_SERCOM_BAUD = 0x04,
        SEESAW_SERCOM_DATA = 0x05,
};

/** neopixel module function addres registers */
enum {
        SEESAW_NEOPIXEL_STATUS = 0x00,
        SEESAW_NEOPIXEL_PIN = 0x01,
        SEESAW_NEOPIXEL_SPEED = 0x02,
        SEESAW_NEOPIXEL_BUF_LENGTH = 0x03,
        SEESAW_NEOPIXEL_BUF = 0x04,
        SEESAW_NEOPIXEL_SHOW = 0x05,
};

/** touch module function addres registers */
enum {
        SEESAW_TOUCH_CHANNEL_OFFSET = 0x10,
};

/** keypad module function addres registers */
enum {
        SEESAW_KEYPAD_STATUS = 0x00,
        SEESAW_KEYPAD_EVENT = 0x01,
        SEESAW_KEYPAD_INTENSET = 0x02,
        SEESAW_KEYPAD_INTENCLR = 0x03,
        SEESAW_KEYPAD_COUNT = 0x04,
        SEESAW_KEYPAD_FIFO = 0x10,
};

/** keypad module edge definitions */
enum {
        SEESAW_KEYPAD_EDGE_HIGH = 0,
        SEESAW_KEYPAD_EDGE_LOW,
        SEESAW_KEYPAD_EDGE_FALLING,
        SEESAW_KEYPAD_EDGE_RISING,
};

/** encoder module edge definitions */
enum {
        SEESAW_ENCODER_STATUS = 0x00,
        SEESAW_ENCODER_INTENSET = 0x02,
        SEESAW_ENCODER_INTENCLR = 0x03,
        SEESAW_ENCODER_POSITION = 0x04,
        SEESAW_ENCODER_DELTA = 0x05,
};

#define SEESAW_HW_ID_CODE			0x55 ///< seesaw HW ID code

/** raw key event stucture for keypad module */
union keyEventRaw {
    struct {
        u8_t EDGE: 2;   ///< the edge that was triggered
        u8_t NUM: 6;    ///< the event number
    } bit;              ///< bitfield format
    u8_t reg;           ///< register format
};


/** extended key event stucture for keypad module */
union keyEvent {
    struct {
        u8_t EDGE: 2;   ///< the edge that was triggered
        u16_t NUM: 14;  ///< the event number
    } bit;              ///< bitfield format
    u16_t reg;          ///< register format
};

/** key state struct that will be written to seesaw chip keypad module */
union keyState {
    struct {
        u8_t STATE: 1;  ///< the current state of the key
        u8_t ACTIVE: 4; ///< the registered events for that key
    } bit;              ///< bitfield format
    u8_t reg;           ///< register format
};

struct seesaw_config {
	char *i2c_name;
#ifdef CONFIG_SEESAW_TRIGGER
	char *gpio_name;
	u8_t gpio_pin;
#endif
	u8_t i2c_address;
};

struct seesaw_data {
	struct device *i2c;
	s16_t sample[64];

#ifdef CONFIG_SEESAW_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t int_handler;
	struct sensor_trigger int_trigger;

#if defined(CONFIG_SEESAW_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_SEESAW_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
	struct k_thread thread;
#elif defined(CONFIG_SEESAW_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_SEESAW_TRIGGER */
};

int seesaw_read(struct device *dev, u8_t regHigh, u8_t regLow,
                u8_t* buf, u8_t num);

#ifdef CONFIG_SEESAW_TRIGGER
int seesaw_attr_set(struct device *dev,
		     enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val);

int seesaw_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int seesaw_init_interrupt(struct device *dev);
#endif /* CONFIG_SEESAW_TRIGGER */

#define DEV_CFG(dev)	\
((const struct seesaw_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) ((struct seesaw_data * const)(dev)->driver_data)

#endif /* ZEPHYR_DRIVERS_SENSOR_SEESAW_SEESAW_H_ */