#ifndef __CONTROLLERS_H
#define __CONTROLLERS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

	typedef enum
	{
		kNotChanging = 0,
		kChangePipeTemperature,
		kChangeMinimalTemperature
	} SettingParamsProcess_t;
	SettingParamsProcess_t Controllers_setParams(void);
	bool Controllers_internalContourProcess(void);
	bool Controllers_externalContourProcess(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONTROLLERS_H */