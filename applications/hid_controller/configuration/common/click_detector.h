#ifndef _CLICK_DETECTOR_H_
#define _CLICK_DETECTOR_H_

#ifdef __cplusplus
extern "C" {
#endif

struct click_detector_config {
	u16_t key_id;
	bool consume_button_event;
};

#ifdef __cplusplus
}
#endif

#endif /* _CLICK_DETECTOR_H_ */