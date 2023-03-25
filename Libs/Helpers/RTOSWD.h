#ifndef __RTOS_WD_H
#define __RTOS_WD_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

	void RTOSWD_init(IWDG_HandleTypeDef* wd);
	void RTOSWD_update(void);
	void RTOSWD_add(void);
	void RTOSWD_remove(void);
	
#ifdef __cplusplus
}
#endif

#endif /* __RTOS_WD_H */