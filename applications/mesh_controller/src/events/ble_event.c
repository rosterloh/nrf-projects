#include <assert.h>
#include <misc/util.h>
#include <stdio.h>

#include "ble_event.h"


static const char * const state_name[] = {
#define X(name) STRINGIFY(name),
	PEER_STATE_LIST
#undef X
};

static int log_ble_peer_event(const struct event_header *eh, char *buf,
			      size_t buf_len)
{
	const struct ble_peer_event *event = cast_ble_peer_event(eh);

	BUILD_ASSERT(ARRAY_SIZE(state_name) == PEER_STATE_COUNT,
		     "Invalid number of elements");

	__ASSERT_NO_MSG(event->state < PEER_STATE_COUNT);

	return snprintf(buf, buf_len, "id=%p %s", event->id,
			state_name[event->state]);
}

static void profile_ble_peer_event(struct log_event_buf *buf,
				   const struct event_header *eh)
{
	const struct ble_peer_event *event = cast_ble_peer_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, (u32_t)event->id);
	profiler_log_encode_u32(buf, event->state);
}

EVENT_INFO_DEFINE(ble_peer_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_U32),
		  ENCODE("conn_id", "state"),
		  profile_ble_peer_event);

EVENT_TYPE_DEFINE(ble_peer_event,
		  IS_ENABLED(CONFIG_MESH_CTL_INIT_LOG_BLE_PEER_EVENT),
		  log_ble_peer_event,
		  &ble_peer_event_info);
