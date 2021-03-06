#ifndef _PORT_STATE_H_
#define _PORT_STATE_H_

#include <stddef.h>

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pin_state {
	u32_t pin;
	u32_t val;
};

struct port_state {
	const char             *name;
	const struct pin_state *ps;
	size_t                  ps_count;
};

#ifdef __cplusplus
}
#endif

#endif /* _PORT_STATE_H_ */