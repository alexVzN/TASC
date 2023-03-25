#ifndef __HC595_H
#define __HC595_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

	void HC595_init(SPI_HandleTypeDef* hspi);
	void HC595_sendBuff(uint8_t* buff, int length, int numbOfDevices);

#ifdef __cplusplus
}
#endif

#endif /* __HC595_H */