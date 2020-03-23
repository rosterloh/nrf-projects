#ifndef _PASSKEY_BUTTONS_H_
#define _PASSKEY_BUTTONS_H_

#include "key_id.h"

#define DIGIT_CNT 10

struct passkey_input_config {
	const u16_t key_ids[DIGIT_CNT];
};

#endif /* _PASSKEY_BUTTONS_H_ */
