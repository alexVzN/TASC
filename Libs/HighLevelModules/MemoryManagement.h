#ifndef __MEMORY_MANAGEMENT_H
#define __MEMORY_MANAGEMENT_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"
	
	void MM_init(void);
	uint64_t* MM_getUids(void);
	void MM_setUids(uint64_t* uids);
	int8_t MM_getMinimalTankTemperature(void);
	void MM_setMinimalTankTemperature(int8_t temperature);
	int8_t MM_getPipeTemperature(void);
	void MM_setPipeTemperature(int8_t temperature);
	void MM_reset(void);
	void MM_updateFlash(void);
	
#ifdef __cplusplus
}
#endif

#endif /* __MEMORY_MANAGEMENT_H */