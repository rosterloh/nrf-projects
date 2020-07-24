#ifndef _SENSOR_EVENT_H_
#define _SENSOR_EVENT_H_

/**
 * @brief Sensor Event
 * @defgroup sensor_event Sensor Event
 * @{
 */

#include <string.h>
#include <toolchain/common.h>
#include <drivers/sensor.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Sensor value event. */
struct sensor_value_event {
	struct event_header header;

	struct sensor_value value;
	enum sensor_channel type;
};

EVENT_TYPE_DECLARE(sensor_value_event);


#ifdef __cplusplus
}
#endif

#endif /* _SENSOR_EVENT_H_ */
