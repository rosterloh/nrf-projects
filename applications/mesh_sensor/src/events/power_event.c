#include "power_event.h"

EVENT_TYPE_DEFINE(power_down_event,
		  IS_ENABLED(CONFIG_MESH_SENSOR_INIT_LOG_POWER_DOWN_EVENT),
		  NULL,
		  NULL);

EVENT_TYPE_DEFINE(wake_up_event,
		  IS_ENABLED(CONFIG_MESH_SENSOR_INIT_LOG_WAKE_UP_EVENT),
		  NULL,
		  NULL);
