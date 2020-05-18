#ifndef _GPIO_PINS_H_
#define _GPIO_PINS_H_

static const char * const port_map[] = {
	DT_LABEL(DT_NODELABEL(gpioa)),
	DT_LABEL(DT_NODELABEL(gpiob)),
	DT_LABEL(DT_NODELABEL(gpioc))
};

struct gpio_pin {
	u8_t port;
	u8_t pin;
};

#endif /* _GPIO_PINS_H_ */
