#ifndef __DS18B20_DRIVER_H
#define __DS18B20_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

#include "stm32f1xx_hal.h"

	typedef enum
	{
		kSearch = 0,
		kMeasurment
	} DS18B20_mode;
	typedef struct
	{
		uint64_t uid;
		int8_t temperature;
	} DS18B20_value;

	void DS18B20Driver_init(TIM_HandleTypeDef* htim);
	void DS18B20Driver_resetDevicesUID(uint64_t* uids, uint8_t len);
	bool DS18B20Driver_addedNewConnectedDeviceUID(DS18B20_value* value);
	void DS18B20Driver_setMode(DS18B20_mode mode);
	int DS18B20Driver_getTemperatures(DS18B20_value* value); // return readed sensors number

#ifdef __cplusplus
}
#endif

#endif /* __DS18B20_DRIVER_H */