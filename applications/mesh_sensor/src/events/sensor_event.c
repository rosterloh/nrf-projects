#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "sensor_event.h"


static int log_sensor_value_event(const struct event_header *eh, char *buf,
				  size_t buf_len)
{
	const struct sensor_value_event *event = cast_sensor_value_event(eh);

	char type_name[11];

	switch (event->type) {
		case SENSOR_CHAN_AMBIENT_TEMP:
			snprintf(type_name, sizeof(type_name), "%s", "temperature");
			break;
		case SENSOR_CHAN_HUMIDITY:
			snprintf(type_name, sizeof(type_name), "%s", "humidity");
			break;
		default:
			snprintf(type_name, sizeof(type_name), "%s", "unknown");
			break;
	}

	return snprintf(buf, buf_len, "%s: %s%u.%06u", type_name,
			((event->value.val1 < 0 || event->value.val2 < 0) ? "-" : ""),
			abs(event->value.val1), abs(event->value.val2));
}

EVENT_TYPE_DEFINE(sensor_value_event,
		  IS_ENABLED(CONFIG_MESH_SENSOR_INIT_LOG_SENSOR_EVENT),
		  log_sensor_value_event,
		  NULL);

