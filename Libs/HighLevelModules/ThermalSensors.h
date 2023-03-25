#ifndef __THERMAL_SENSORS_H
#define __THERMAL_SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"
	
	typedef enum {
		kNotFound = -75,
		kNotRead = - 100
	} ThermalSensors_status_t;
	
	void ThermalSensors_init(TIM_HandleTypeDef* htim, uint64_t* uids);
	uint64_t* ThermalSensors_uids(void);
	int8_t ThermalSensors_watertank(int index);
	int8_t ThermalSensors_boiler(void);
	int8_t ThermalSensors_pipe(void);
	void ThermalSensors_reset(void);
	void ThermalSensors_startMatch(void);
	void ThermalSensors_stopMatch(void);
	uint8_t ThermalSensors_matchNext(void); // 0 -- (watertank) -- 6 -- 7 (boiler) -- 8 (pipe)
	int8_t ThermalSensors_newMatchTemperature(void);

#ifdef __cplusplus
}
#endif

#endif /* __THERMAL_SENSORS_H */