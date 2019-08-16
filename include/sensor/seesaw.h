#ifndef ZEPHYR_INCLUDE_SEESAW_H_
#define ZEPHYR_INCLUDE_SEESAW_H_

#include <sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

enum seesaw_attribute {
	SEESAW_ATTR_TBD = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SEESAW_H_ */
