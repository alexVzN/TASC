#include "MainLogic.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "cmsis_os.h"
#include "main.h"

#include "Helpers/RTOSWD.h"
#include "DeviceDrivers/DS18B20Driver.h"
#include "DeviceDrivers/IndicatorsDriver.h"
#include "DeviceDrivers/StateInputs.h"
#include "DeviceDrivers/Outputs.h"
#include "HighLevelModules/ThermalSensors.h"
#include "HighLevelModules/MemoryManagement.h"
#include "HighLevelModules/Controllers.h"

#define BLINK_MAX_COUNT_S	3
int matchPosition = 0;
int blinkTimeoutCounter = 0;
int8_t matcedValue = kNotFound;

void ML_match(void)
{
	int prevPosition = matchPosition;
	if (StateInputs_switchMatchTarget()) {
		matcedValue = kNotFound;
		matchPosition = ThermalSensors_matchNext();
		if (matchPosition < 7)
		{
			printf("Match tank sensor %d\n", matchPosition + 1);
		}
		else if (matchPosition == 7)
		{
			printf("Match boiler sensor\n");
		}
		else
		{
			printf("Match pipe sensor\n");
		}
	}
	
	if (prevPosition != matchPosition) {
		IndicatorsDriver_setStatusForAll(kOff);
	}
	
	int8_t updateMatchedValue = ThermalSensors_newMatchTemperature();
	if (updateMatchedValue != kNotFound || matcedValue == kNotFound) {
		matcedValue = updateMatchedValue;
	}
	
	if (matchPosition < 7) {
		
		if (matcedValue == kNotFound) {
			IndicatorsDriver_setSmallNumber(matchPosition, 0, kLoad);
		} else if (matcedValue == kNotRead) {
			printf("Matching error\n");
			IndicatorsDriver_setSmallNumber(matchPosition, 0, kError);
		} else {
			printf("Matching successful\n");
			IndicatorsDriver_setSmallNumber(matchPosition, matcedValue, kStandart);
		}
	} else {

		if (matcedValue == kNotFound) {
			IndicatorsDriver_setBigNumber(0, kLoad);
		} else if (matcedValue == kNotRead) {
			printf("Matching error\n");
			IndicatorsDriver_setBigNumber(0, kError);
		} else {
			printf("Matching successful\n");
			IndicatorsDriver_setBigNumber(matcedValue, kStandart);
		}

		IndicatorsDriver_setLed(matchPosition - 7, kStandart);
	}
}

void ML_controll(void)
{
	SettingParamsProcess_t settingStatus = Controllers_setParams();
	int startCount = 0;
	if (settingStatus == kChangeMinimalTemperature) {
		IndicatorsDriver_setSmallNumber(0, MM_getMinimalTankTemperature(), kFastBlink);
		startCount = 1;
	}

	if (ThermalSensors_watertank(0) <= MM_getMinimalTankTemperature() && ThermalSensors_watertank(0) > 0) {
		for (int i = startCount; i < 7; i++)
		{
			IndicatorsDriver_setSmallNumber(i, 0, kEmpty);
		}
	} else {
		for (int i = startCount; i < 7; i++)
		{
			int8_t value = ThermalSensors_watertank(i);
			if (value == kNotRead)
			{
				IndicatorsDriver_setSmallNumber(i, 0, kError);
			}
			else
			{
				IndicatorsDriver_setSmallNumber(i, value, kStandart);
			}
		}
	}

	{
		bool status = Controllers_externalContourProcess();
		IndicatorsDriver_setLed(1, status ? Outputs_getOutsidePumpStatus() ? kStandart : kOff : kBlink);

		if (settingStatus == kChangePipeTemperature) {
			IndicatorsDriver_setBigNumber(MM_getPipeTemperature(), kFastBlink);
		} else {
			int8_t value = ThermalSensors_pipe();
			if (value == kNotRead) {
				IndicatorsDriver_setBigNumber(0, kError);
			}
			else {
				IndicatorsDriver_setBigNumber(value, kStandart);
			}
		}
		
		
		volatile float valvePosition = Outputs_get3WayValveCurrentPosition();
		volatile uint8_t indicatorPosition = roundf(valvePosition / 10.0);

		IndicatorsDriver_setProgress(1, indicatorPosition, kStandart);
	}

	{
		bool status = Controllers_internalContourProcess();
		IndicatorsDriver_setLed(0, status ? Outputs_getInsidePumpStatus() ? kStandart : kOff : kBlink);
	}
}

bool ML_startMatch(void)
{
	if (blinkTimeoutCounter == 0)
	{
		printf("\nStart match\n");
		blinkTimeoutCounter = osKernelGetSysTimerCount();
		IndicatorsDriver_setStatusForAll(kBlink);
	} else if (osKernelGetSysTimerCount() - blinkTimeoutCounter > BLINK_MAX_COUNT_S * osKernelGetSysTimerFreq()) {
		IndicatorsDriver_setStatusForAll(kOff);
		matchPosition = 0;
		ThermalSensors_startMatch();
		return true;
	}

	return false;
}

bool ML_stopMatch(void)
{
	printf("\nStop match\n");
	ThermalSensors_stopMatch();
	blinkTimeoutCounter = 0;
	MM_setUids(ThermalSensors_uids());
	return true;
}
