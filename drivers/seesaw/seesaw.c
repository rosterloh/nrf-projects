#include <string.h>
#include <device.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <kernel.h>
#include <sys/__assert.h>

#include "seesaw.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(seesaw, CONFIG_SENSOR_LOG_LEVEL);

int seesaw_read(struct device *dev, u8_t regHigh, u8_t regLow,
                u8_t *buf, u8_t num, u16_t delay)
{
	struct seesaw_data *data = DEV_DATA(dev);
        const struct seesaw_config *config = DEV_CFG(dev);
        int ret;

	u8_t reg[2] = { regHigh, regLow };

        ret = i2c_write(data->i2c, reg, 2, config->i2c_address);

        k_busy_wait(delay);

	return i2c_read(data->i2c, buf, num, config->i2c_address);
}

int seesaw_write(struct device *dev, u8_t regHigh, u8_t regLow,
                u8_t *buf, u8_t num)
{
	struct seesaw_data *data = DEV_DATA(dev);
        const struct seesaw_config *config = DEV_CFG(dev);

	u8_t reg[6] = { regHigh, regLow };

        memcpy(&reg[2], buf, num);

        return i2c_write(data->i2c, reg, num+2, config->i2c_address);
}

static int seesaw_set_pin_mode(struct device *dev, u32_t pins, u8_t mode)
{
        u8_t cmd[] = { (u8_t)(pins >> 24) , (u8_t)(pins >> 16),
                       (u8_t)(pins >> 8), (u8_t)pins };
        switch (mode) {
		case OUTPUT:
                        seesaw_write(dev, SEESAW_GPIO_BASE,
                                     SEESAW_GPIO_DIRSET_BULK, cmd, 4);
			break;
		case INPUT:
			seesaw_write(dev, SEESAW_GPIO_BASE,
                                     SEESAW_GPIO_DIRCLR_BULK, cmd, 4);
			break;
		case INPUT_PULLUP:
			seesaw_write(dev, SEESAW_GPIO_BASE,
                                     SEESAW_GPIO_DIRCLR_BULK, cmd, 4);
			seesaw_write(dev, SEESAW_GPIO_BASE,
                                     SEESAW_GPIO_PULLENSET, cmd, 4);
			seesaw_write(dev, SEESAW_GPIO_BASE,
                                     SEESAW_GPIO_BULK_SET, cmd, 4);
			break;
		case INPUT_PULLDOWN:
			seesaw_write(dev, SEESAW_GPIO_BASE,
                                     SEESAW_GPIO_DIRCLR_BULK, cmd, 4);
			seesaw_write(dev, SEESAW_GPIO_BASE,
                                     SEESAW_GPIO_PULLENSET, cmd, 4);
			seesaw_write(dev, SEESAW_GPIO_BASE,
                                     SEESAW_GPIO_BULK_CLR, cmd, 4);
			break;
	}

	return 0;
}

static int seesaw_get_digital(struct device *dev, u32_t pins, u32_t *val)
{
        u8_t buf[4];

        seesaw_read(dev, SEESAW_GPIO_BASE, SEESAW_GPIO_BULK, buf, 4, 170);

        *val = sys_get_be32(buf) & pins;

	return 0;
}

static int seesaw_get_analog(struct device *dev, u8_t pin, u16_t *val)
{
        u8_t buf[2];
        u8_t p;

        switch (pin) {
		case ADC_INPUT_0_PIN: p = 0; break;
		case ADC_INPUT_1_PIN: p = 1; break;
		case ADC_INPUT_2_PIN: p = 2; break;
		case ADC_INPUT_3_PIN: p = 3; break;
		default:
                        LOG_WRN("Unknown analog pin read requested (%d)", pin);
			return 0;
			break;
	}

        seesaw_read(dev, SEESAW_ADC_BASE, SEESAW_ADC_CHANNEL_OFFSET + p, buf, 2, 500);

        *val = sys_get_be16(buf);

        return 0;
}

static int seesaw_set_gpio_interrupts(struct device *dev, u32_t pins, u8_t en)
{
        u8_t cmd[] = { (u8_t)(pins >> 24) , (u8_t)(pins >> 16),
                       (u8_t)(pins >> 8), (u8_t)pins };
	if (en) {
		seesaw_write(dev, SEESAW_GPIO_BASE,
                             SEESAW_GPIO_INTENSET, cmd, 4);
	} else {
		seesaw_write(dev, SEESAW_GPIO_BASE,
                             SEESAW_GPIO_INTENCLR, cmd, 4);
        }

        return 0;
}

static int seesaw_init_device(struct device *dev)
{
        struct seesaw_data *data = DEV_DATA(dev);
        const struct seesaw_config *config = DEV_CFG(dev);
        u8_t hw_id;
        u8_t buf[4];

        k_sleep(200);
        u8_t reg[3] = { SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, 0xFF };
        if (i2c_write(data->i2c, reg, 3, config->i2c_address) < 0) {
                LOG_ERR("Failed to software reset device");
		return -EIO;
        }
        k_sleep(500);

        if (seesaw_read(dev, SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID,
                        &hw_id, 1, 170) < 0) {
		LOG_ERR("Cannot obtain hardware ID");
		return -EIO;
	}

        if (hw_id != SEESAW_HW_ID_CODE) {
                LOG_ERR("Unknown device deteced: 0x%" PRIx8, hw_id);
                return -EFAULT;
        }        

	if (seesaw_read(dev, SEESAW_STATUS_BASE, SEESAW_STATUS_VERSION,
                        buf, 4, 170) < 0) {
		LOG_ERR("Cannot obtain version information");
		return -EIO;
	}

        u32_t version = sys_get_be32(buf);
        u16_t date = version & 0xFFFF;
        data->pid = version >> 16;
	LOG_DBG("Product ID: %d, Compiled: %d-%d-20%d",
                data->pid, date & 0x1F, (date & 0x1E0) >> 5, date >> 9);

        if (seesaw_read(dev, SEESAW_STATUS_BASE, SEESAW_STATUS_OPTIONS,
                        buf, 4, 170) < 0) {
		LOG_ERR("Cannot obtain version information");
		return -EIO;
	}

        u32_t options = sys_get_be32(buf);
        LOG_DBG("Modules found:");
                
        if ((options & (1UL << SEESAW_TIMER_BASE)) > 0)
                LOG_DBG("\tTIMER");
        if ((options & (1UL << SEESAW_ADC_BASE)) > 0)
                LOG_DBG("\tADC");
        if ((options & (1UL << SEESAW_DAC_BASE)) > 0)
                LOG_DBG("\tDAC");
        if ((options & (1UL << SEESAW_INTERRUPT_BASE)) > 0)
                LOG_DBG("\tINTERRUPT");
        if ((options & (1UL << SEESAW_DAP_BASE)) > 0)
                LOG_DBG("\tDAP");
        if ((options & (1UL << SEESAW_EEPROM_BASE)) > 0)
                LOG_DBG("\tEEPROM");
        if ((options & (1UL << SEESAW_NEOPIXEL_BASE)) > 0)
                LOG_DBG("\tNEOPIXEL");
        if ((options & (1UL << SEESAW_TOUCH_BASE)) > 0)
                LOG_DBG("\tTOUCH");
        if ((options & (1UL << SEESAW_KEYPAD_BASE)) > 0)
                LOG_DBG("\tKEYPAD");
        if ((options & (1UL << SEESAW_ENCODER_BASE)) > 0)
                LOG_DBG("\tENCODER");

	return 0;
}

static int seesaw_init(struct device *dev)
{
	struct seesaw_data *data = DEV_DATA(dev);
        const struct seesaw_config *config = DEV_CFG(dev);

        data->i2c = device_get_binding(config->i2c_name);
	if (data->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			config->i2c_name);
		return -EINVAL;
	}

        if (seesaw_init_device(dev) < 0) {
		LOG_ERR("Failed to initialise device!");
		return -EIO;
	}

#ifdef CONFIG_SEESAW_TRIGGER
	if (seesaw_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialise interrupt!");
		return -EIO;
	}
#endif
	return 0;
}

static const struct seesaw_driver_api seesaw_driver_api = {
        .write_pin_mode = seesaw_set_pin_mode,
        .read_digital = seesaw_get_digital,
        .read_analog = seesaw_get_analog,
        .gpio_interrupts = seesaw_set_gpio_interrupts,
        .int_set = seesaw_set_int_callback,
};

static const struct seesaw_config seesaw_config = {
	.i2c_name = DT_INST_0_ADAFRUIT_SEESAW_BUS_NAME,
	.i2c_address = DT_INST_0_ADAFRUIT_SEESAW_BASE_ADDRESS,
#ifdef CONFIG_SEESAW_TRIGGER
	.gpio_name = DT_INST_0_ADAFRUIT_SEESAW_INT_GPIOS_CONTROLLER,
	.gpio_pin = DT_INST_0_ADAFRUIT_SEESAW_INT_GPIOS_PIN,
#endif
};

static struct seesaw_data seesaw_driver;

DEVICE_AND_API_INIT(seesaw, DT_INST_0_ADAFRUIT_SEESAW_LABEL, seesaw_init,
		    &seesaw_driver, &seesaw_config, 
                    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &seesaw_driver_api);
