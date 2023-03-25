#include "ThermalSensors.h"

#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "DeviceDrivers/DS18B20Driver.h"

#pragma pack(push, 1)
struct {
	uint64_t watertank[7];
	uint64_t boiler;
	uint64_t pipe;
} thermalSensorsAdd;
#pragma pack(pop)

int matchIndex = 0;

int8_t getTemperatureByUid(uint64_t uid)
{
	DS18B20_value values[24];
	int size = DS18B20Driver_getTemperatures(values);
	for (int i = 0; i < 24; i++)
	{
		if (values[i].uid == uid)
		{
			return values[i].temperature;
		}
	}
	
	return -100;
}

void ThermalSensors_init(TIM_HandleTypeDef* htim, uint64_t* uids)
{
	memcpy(&thermalSensorsAdd, uids, sizeof(thermalSensorsAdd));
	DS18B20Driver_init(htim);
	DS18B20Driver_resetDevicesUID(uids, 9);
}

uint64_t* ThermalSensors_uids(void)
{
	return (uint64_t*)&thermalSensorsAdd;
}

int8_t ThermalSensors_watertank(int index)
{
	return getTemperatureByUid(thermalSensorsAdd.watertank[index]);
}

int8_t ThermalSensors_boiler(void)
{
	return getTemperatureByUid(thermalSensorsAdd.boiler);
}

int8_t ThermalSensors_pipe(void)
{
	return getTemperatureByUid(thermalSensorsAdd.pipe);
}

void ThermalSensors_reset(void)
{
	memset(&thermalSensorsAdd, 0, sizeof(thermalSensorsAdd));
}

void ThermalSensors_startMatch(void)
{
	DS18B20Driver_setMode(kSearch);
	matchIndex = 0;
}

void ThermalSensors_stopMatch(void)
{
	DS18B20Driver_setMode(kMeasurment);
}

uint8_t ThermalSensors_matchNext(void)
{
	matchIndex++;
	if (matchIndex == sizeof(thermalSensorsAdd) / sizeof(uint64_t))
		matchIndex = 0;
	
	return matchIndex;
}

int8_t ThermalSensors_newMatchTemperature(void)
{
	DS18B20_value value;
	if (DS18B20Driver_addedNewConnectedDeviceUID(&value))
	{
		*(((uint64_t*)&thermalSensorsAdd) + matchIndex) = value.uid;
		return value.temperature;
	}

	return kNotFound;
}
