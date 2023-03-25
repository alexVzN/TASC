#ifndef __OUTPUTS_H
#define __OUTPUTS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#include "stm32f1xx_hal.h"
	
	void Outputs_init(void);

	void Outputs_setInsidePumpStatus(bool enable);
	bool Outputs_getInsidePumpStatus(void);
	void Outputs_setOutsidePumpStatus(bool enable);
	bool Outputs_getOutsidePumpStatus(void);
	
	void Outputs_set3WayValvePosition(uint8_t position);
	uint8_t Outputs_get3WayValveSetPosition(void);
	uint8_t Outputs_get3WayValveCurrentPosition(void);
	void Outputs_valve3WayResetPosition(void);
	bool Outputs_get3WayValveOnResettingPosition(void);
	bool Outputs_get3WayValveOnProcess(void);

#ifdef __cplusplus
}
#endif

#endif /* __OUTPUTS_H */