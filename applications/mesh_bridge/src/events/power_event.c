#include "power_event.h"

EVENT_TYPE_DEFINE(power_down_event,
		  IS_ENABLED(CONFIG_BRIDGE_LOG_POWER_DOWN_EVENT),
		  NULL,
		  NULL);
