/**
 * @file drivers/seesaw.h
 *
 * @brief Public APIs for the SeeSaw driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SEESAW_H_
#define ZEPHYR_INCLUDE_DRIVERS_SEESAW_H_

/**
 * @brief SeeSaw Interface
 * @{
 */

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
        INPUT= 0x0,
        OUTPUT = 0x1,
        INPUT_PULLUP = 0x2,
        INPUT_PULLDOWN = 0x3,
};

/**
 * @typedef seesaw_configure_gpio_t
 * @brief Callback API to configure seesaw gpio.
 *
 * See seesaw_gpio_configure() for argument description
 */
typedef int (*seesaw_configure_gpio_t)(struct device *dev, u32_t pins, u8_t mode);

/**
 * @typedef seesaw_read_digital_t
 * @brief Callback API for reading seesaw gpio pins.
 *
 * See seesaw_read_digital() for argument description
 */
typedef int (*seesaw_read_digital_t)(struct device *dev, u32_t pins, u32_t *val);

/**
 * @typedef seesaw_read_analog_t
 * @brief Callback API for reading an seesaw analog pin.
 *
 * See seesaw_read_analog() for argument description
 */
typedef int (*seesaw_read_analog_t)(struct device *dev, u8_t pin, u16_t *val);

/**
 * @typedef seesaw_interrupts_gpio_t
 * @brief Callback API to configure seesaw gpio interrupts.
 *
 * See seesaw_gpio_interrupts() for argument description
 */
typedef int (*seesaw_interrupts_gpio_t)(struct device *dev, u32_t pins,
                                        u8_t enable);

/**
 * @typedef seesaw_int_callback_t
 * @brief Define the callback function for interrupts 
 *
 * @param "struct device *dev" Pointer to the seesaw device
 */
typedef void (*seesaw_int_callback_t)(struct device *dev);

/**
 * @typedef seesaw_int_set_t
 * @brief Callback API for setting a seesaw's interrupt handler
 *
 * See seesaw_int_set() for argument description
 */
typedef int (*seesaw_int_set_t)(struct device *dev,
				seesaw_int_callback_t handler);

struct seesaw_driver_api {
	seesaw_configure_gpio_t write_pin_mode;
        seesaw_read_digital_t read_digital;
        seesaw_read_analog_t read_analog;
        seesaw_interrupts_gpio_t gpio_interrupts;
        seesaw_int_set_t int_set;
};

/**
 * @brief Configures the gpio of a seesaw device.
 *
 * @param dev Pointer to the seesaw device.
 * @param pins Button mask of pins to configure.
 * @param mode Mode to configure pins as.
 * 
 * @retval 0 on success.
 * @retval -ERRNO errno code on error.
 */
static inline int seesaw_gpio_configure(struct device *dev, u32_t pins, u8_t mode)
{
	const struct seesaw_driver_api *api =
		(const struct seesaw_driver_api *)dev->driver_api;

	__ASSERT(api->write_pin_mode != NULL,
		"Callback pointer should not be NULL");
	return api->write_pin_mode(dev, pins, mode);
}

/**
 * @brief Reads the state of the gpios of a seesaw device.
 *
 * @param dev Pointer to the seesaw device.
 * @param pins Button mask of pins to read.
 * 
 * @retval 0 on success.
 * @retval -ERRNO errno code on error.
 */
static inline int seesaw_read_digital(struct device *dev, u32_t pins, u32_t *val)
{
	const struct seesaw_driver_api *api =
		(const struct seesaw_driver_api *)dev->driver_api;

	__ASSERT(api->read_digital != NULL,
		"Callback pointer should not be NULL");
	return api->read_digital(dev, pins, val);
}

/**
 * @brief Reads the value of a seesaw analog pin.
 *
 * @param dev Pointer to the seesaw device.
 * @param pin Analog pin to read.
 * 
 * @retval 0 on success.
 * @retval -ERRNO errno code on error.
 */
static inline int seesaw_read_analog(struct device *dev, u8_t pin, u16_t *val)
{
	const struct seesaw_driver_api *api =
		(const struct seesaw_driver_api *)dev->driver_api;

	__ASSERT(api->read_analog != NULL,
		"Callback pointer should not be NULL");
	return api->read_analog(dev, pin, val);
}

/**
 * @brief Configures the gpio interrupts of a seesaw device.
 *
 * @param dev Pointer to the seesaw device.
 * @param pins Button mask of pins to configure.
 * @param enabled 0 to disable or 1 to enable interrupts.
 * 
 * @retval 0 on success.
 * @retval -ERRNO errno code on error.
 */
static inline int seesaw_gpio_interrupts(struct device *dev, u32_t pins, u8_t enabled)
{
        const struct seesaw_driver_api *api =
		(const struct seesaw_driver_api *)dev->driver_api;

	__ASSERT(api->gpio_interrupts != NULL,
		"Callback pointer should not be NULL");
	return api->gpio_interrupts(dev, pins, enabled);
}

/**
 * @brief Set the INT callback function pointer.
 *
 * This sets up the callback for INT. When an IRQ is triggered,
 * the specified function will be called with the device pointer.
 *
 * @param dev Seesaw device structure.
 * @param cb Pointer to the callback function.
 *
 * @return N/A
 */
static inline void seesaw_int_set(struct device *dev,
				  seesaw_int_callback_t cb)
{
	const struct seesaw_driver_api *api =
		(const struct seesaw_driver_api *)dev->driver_api;

	if ((api != NULL) && (api->int_set != NULL)) {
		api->int_set(dev, cb);
	}
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SEESAW_H_ */
