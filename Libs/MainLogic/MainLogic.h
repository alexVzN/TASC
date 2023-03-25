#ifndef __MAIN_LOGIC_H
#define __MAIN_LOGIC_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

	bool ML_startMatch(void);
	bool ML_stopMatch(void);
	void ML_match(void);
	void ML_controll(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_LOGIC_H */