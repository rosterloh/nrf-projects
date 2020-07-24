#ifndef _GPIO_PINS_H_
#define _GPIO_PINS_H_


static const char * const port_map[] = {
#ifdef CONFIG_GPIO_NRF_P0
	DT_LABEL(DT_NODELABEL(gpio0)),
#else
	NULL,
#endif /* CONFIG_GPIO_NRF_P0 */
#ifdef CONFIG_GPIO_SX1509B
	DT_LABEL(DT_NODELABEL(sx1509b)),
#else
	NULL,
#endif /* CONFIG_GPIO_SX1509B */
};

struct gpio_pin {
	uint8_t port;
	uint8_t pin;
};

#endif /* _GPIO_PINS_H_ */
