#include <event_manager.h>
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE);


void main(void)
{
	if (event_manager_init()) {
		LOG_ERR("Event Manager not initialised");
	} else {
		module_set_state(MODULE_STATE_READY);
	}
}
