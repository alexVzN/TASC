#ifndef __STATE_INPUTS_H
#define __STATE_INPUTS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#include "stm32f1xx_hal.h"
	
	void StateInputs_init(TIM_HandleTypeDef* htim);
	
	int StateInputs_getEncoderValue(void);
	bool StateInputs_isMatchMode(void);
	bool StateInputs_switchMatchTarget(void);
	bool StateInputs_onClickedMainButton(void);
	bool StateInputs_termostateStatus(void);
	bool StateInputs_resetDevice(void);
	

#ifdef __cplusplus
}
#endif

#endif /* __STATE_INPUTS_H */