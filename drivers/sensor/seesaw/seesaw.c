#include <string.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "seesaw.h"

LOG_MODULE_REGISTER(seesaw, CONFIG_SENSOR_LOG_LEVEL);

static int seesaw_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	//struct seesaw_data *data = DEV_DATA(dev);
        //const struct seesaw_config *config = DEV_CFG(dev);

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
/*
	if (i2c_burst_read(data->i2c, config->i2c_address,
			   SEESAW_OUTPUT_BASE,
			   (u8_t *)data->sample,
			   sizeof(data->sample))) {
		return -EIO;
	}
*/
	return 0;
}

static int seesaw_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	//struct seesaw_data *data = DEV_DATA(dev);
	//size_t len = ARRAY_SIZE(data->sample);

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	return 0;
}

int seesaw_read(struct device *dev, u8_t regHigh, u8_t regLow,
                u8_t* buf, u8_t num)
{
	struct seesaw_data *data = DEV_DATA(dev);
        const struct seesaw_config *config = DEV_CFG(dev);
        int ret;

	u8_t reg[2] = { regHigh, regLow };

        ret = i2c_write(data->i2c, reg, 2, config->i2c_address);

        k_busy_wait(170);

	return i2c_read(data->i2c, buf, num, config->i2c_address);
}

static int seesaw_init_device(struct device *dev)
{
        struct seesaw_data *data = DEV_DATA(dev);
        const struct seesaw_config *config = DEV_CFG(dev);
        u8_t hw_id;
        u8_t buf[4];
        
        u8_t reg[3] = { SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST, 0xFF };
        if (i2c_write(data->i2c, reg, 3, config->i2c_address) < 0) {
                LOG_ERR("Failed to software reset device");
		return -EIO;
        }
        k_sleep(500);

        if (seesaw_read(dev, SEESAW_STATUS_BASE, SEESAW_STATUS_HW_ID,
                        &hw_id, 1) < 0) {
		LOG_ERR("Cannot obtain hardware ID");
		return -EIO;
	}

        if (hw_id != SEESAW_HW_ID_CODE) {
                LOG_ERR("Unknown device deteced: 0x%" PRIx8, hw_id);
                return -EFAULT;
        }        

	if (seesaw_read(dev, SEESAW_STATUS_BASE, SEESAW_STATUS_VERSION,
                        buf, 4) < 0) {
		LOG_ERR("Cannot obtain version information");
		return -EIO;
	}

        u32_t version = sys_get_be32(buf);
        u16_t date = version & 0xFFFF;
	LOG_DBG("Product ID: %d, Compiled: %d-%d-20%d", version >> 16, date & 0x1F, (date & 0x1E0) >> 5, date >> 9);

        if (seesaw_read(dev, SEESAW_STATUS_BASE, SEESAW_STATUS_OPTIONS,
                        buf, 4) < 0) {
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

struct seesaw_data seesaw_driver;

static const struct sensor_driver_api seesaw_driver_api = {
#ifdef CONFIG_SEESAW_TRIGGER
	.attr_set     = seesaw_attr_set,
	.trigger_set  = seesaw_trigger_set,
#endif
	.sample_fetch = seesaw_sample_fetch,
	.channel_get  = seesaw_channel_get,
};

static const struct seesaw_config seesaw_config = {
	.i2c_name = DT_INST_0_ADAFRUIT_SEESAW_BUS_NAME,
	.i2c_address = DT_INST_0_ADAFRUIT_SEESAW_BASE_ADDRESS,
#ifdef CONFIG_SEESAW_TRIGGER
	.gpio_name = DT_INST_0_ADAFRUIT_SEESAW_INT_GPIOS_CONTROLLER,
	.gpio_pin = DT_INST_0_ADAFRUIT_SEESAW_INT_GPIOS_PIN,
#endif
};

DEVICE_AND_API_INIT(seesaw, DT_INST_0_ADAFRUIT_SEESAW_LABEL, seesaw_init,
		    &seesaw_driver, &seesaw_config, 
                    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &seesaw_driver_api);
