#include "Controllers.h"

#include "DeviceDrivers/StateInputs.h"
#include "DeviceDrivers/IndicatorsDriver.h"
#include "DeviceDrivers/Outputs.h"
#include "HighLevelModules/ThermalSensors.h"
#include "HighLevelModules/MemoryManagement.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cmsis_os.h"

#define CHANGE_VALUES_TIMEOUT_S 5
#define CHANGE_VALVE_TIMEOUT_S 3

uint32_t countdownCounter = 0;
uint32_t valveTimeoutCounter = 0;
SettingParamsProcess_t changeValues = kNotChanging;

void resetCountdown(void)
{
	countdownCounter = osKernelGetTickCount() + CHANGE_VALUES_TIMEOUT_S * osKernelGetTickFreq();
}

void disableContour(void)
{
	if (Outputs_getOutsidePumpStatus()) {
		Outputs_setOutsidePumpStatus(false);
	}
	
	if (!Outputs_get3WayValveOnResettingPosition() && Outputs_get3WayValveCurrentPosition() != 0) {
		Outputs_valve3WayResetPosition();
	}
}

void enableContour(int8_t pipeTemperature, int8_t watertankTemperature)
{
	int pipeLackTemperature = MM_getPipeTemperature() - pipeTemperature;
	if (!Outputs_getOutsidePumpStatus()) {
		Outputs_setOutsidePumpStatus(true);
		Outputs_set3WayValvePosition(pipeLackTemperature);
	}

	if (osKernelGetSysTimerCount() - valveTimeoutCounter < CHANGE_VALVE_TIMEOUT_S * osKernelGetSysTimerFreq())
	{
		return;
	}
	
	valveTimeoutCounter = osKernelGetSysTimerCount();
	
	if (Outputs_get3WayValveOnProcess() || abs(pipeLackTemperature) < 2)
	{
		return;
	}
	
	int currentPosition = Outputs_get3WayValveCurrentPosition();
	if (MM_getPipeTemperature() > watertankTemperature) {
		Outputs_set3WayValvePosition(100);
	} else if (MM_getPipeTemperature() == pipeTemperature) {
		Outputs_set3WayValvePosition(currentPosition);
	} else if (!Outputs_get3WayValveOnProcess()){
		currentPosition += pipeLackTemperature;
		if (currentPosition < 0)
			currentPosition = 0;
		Outputs_set3WayValvePosition(currentPosition);
	}
}

SettingParamsProcess_t Controllers_setParams(void)
{
	if (StateInputs_onClickedMainButton())
	{
		if (changeValues == kNotChanging)
		{
			changeValues = kChangePipeTemperature;
		}
		else
		{
			changeValues = changeValues == kChangePipeTemperature ? kChangeMinimalTemperature : kChangePipeTemperature;
		}
		
		resetCountdown();
	}

	if (changeValues != kNotChanging)
	{
		int valueForUpdating = StateInputs_getEncoderValue();
		if (valueForUpdating != 0) {
			resetCountdown();

			if (changeValues == kChangePipeTemperature) {
				int value = MM_getPipeTemperature() + valueForUpdating;
				MM_setPipeTemperature(value);
			} else if (changeValues == kChangeMinimalTemperature) {
				int value = MM_getMinimalTankTemperature() + valueForUpdating;
				MM_setMinimalTankTemperature(value);
			}
		}
	}
	
	if (osKernelGetTickCount() > countdownCounter) {
		changeValues = kNotChanging;
		MM_updateFlash();
	}

	return changeValues;
}

bool Controllers_internalContourProcess(void)
{
	int8_t boilerTemperature = ThermalSensors_boiler();
	int8_t waterrtankTemperature = ThermalSensors_watertank(0);

	if (boilerTemperature == kNotRead || waterrtankTemperature == kNotRead)
	{
		Outputs_setInsidePumpStatus(true);
		return false;
	}
	
	if (boilerTemperature > waterrtankTemperature) {
		if (!Outputs_getInsidePumpStatus()) {
			Outputs_setInsidePumpStatus(true);
		}
	} else {
		if (Outputs_getInsidePumpStatus()) {
			Outputs_setInsidePumpStatus(false);
		}
	}
	
	return true;
}

bool Controllers_externalContourProcess(void)
{
	int8_t pipeTemperature = ThermalSensors_pipe();
	int8_t waterrtankTemperature = ThermalSensors_watertank(0);
	if (pipeTemperature == kNotRead || waterrtankTemperature == kNotRead)
	{
		Outputs_setOutsidePumpStatus(false);
		return false;
	}
	
	if (waterrtankTemperature > MM_getMinimalTankTemperature() && StateInputs_termostateStatus()) {
		enableContour(pipeTemperature, waterrtankTemperature);
	} else {
		disableContour();
	}
	
	return true;
}
