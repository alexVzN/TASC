#include "Outputs.h"

#include "main.h"
#include "cmsis_os.h"

#include <stdio.h>

#include "Helpers/RTOSWD.h"

#define FULL_CICLE_TIME_S	35.0

#define VALVE_UP()		HAL_GPIO_WritePin(CH1_SSR_GPIO_Port, CH1_SSR_Pin, GPIO_PIN_RESET)
#define VALVE_DOWN()	HAL_GPIO_WritePin(CH0_SSR_GPIO_Port, CH0_SSR_Pin, GPIO_PIN_RESET)
#define VALVE_STOP()	HAL_GPIO_WritePin(CH0_SSR_GPIO_Port, CH0_SSR_Pin, GPIO_PIN_SET); \
						HAL_GPIO_WritePin(CH1_SSR_GPIO_Port, CH1_SSR_Pin, GPIO_PIN_SET)

uint8_t currentPosition = 0;
uint8_t setPosition = 0;

bool isResetting = false;

enum
{
	kUp,
	kDown,
	kNone
} valveProcess;

/* Definitions for valveControllerTask */
osThreadId_t valveControllerHandle;
const osThreadAttr_t valveController_attributes = {
	.name = "valveController",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityNormal,
};

void valveControllerTask(void *argument);

/*Definition for Mutex*/
const osMutexDef_t valveMutex_attribute = {
	.name = "valveMutex"
};

osMutexId valveMutexId;

void Outputs_init(void)
{
	valveMutexId = osMutexNew(&valveMutex_attribute);
	valveControllerHandle = osThreadNew(&valveControllerTask, NULL, &valveController_attributes);
	Outputs_valve3WayResetPosition();
	isResetting = true;
}

void Outputs_setInsidePumpStatus(bool enable)
{
	printf(enable ? "Inside pump enable\n" : "Inside pump disable\n");
	HAL_GPIO_WritePin(CH3_SSR_GPIO_Port, CH3_SSR_Pin, enable ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

bool Outputs_getInsidePumpStatus(void)
{
	return HAL_GPIO_ReadPin(CH3_SSR_GPIO_Port, CH3_SSR_Pin) == GPIO_PIN_RESET;
}

void Outputs_setOutsidePumpStatus(bool enable)
{
	printf(enable ? "Outside pump enable\n" : "Outside pump disable\n");
	HAL_GPIO_WritePin(CH2_SSR_GPIO_Port, CH2_SSR_Pin, enable ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

bool Outputs_getOutsidePumpStatus(void)
{
	return HAL_GPIO_ReadPin(CH2_SSR_GPIO_Port, CH2_SSR_Pin) == GPIO_PIN_RESET;
}

void Outputs_set3WayValvePosition(uint8_t position)
{
	printf("Set valve position: %d/100\n", position);
	if (position > 130)
		return;

	osMutexWait(valveMutexId, portMAX_DELAY);
	setPosition = position > 100 ? 100 : position;		
	osMutexRelease(valveMutexId);
}

uint8_t Outputs_get3WayValveSetPosition(void)
{
	return setPosition;
}

uint8_t Outputs_get3WayValveCurrentPosition(void)
{
	return currentPosition;
}

void Outputs_valve3WayResetPosition(void)
{
	osMutexWait(valveMutexId, portMAX_DELAY);
	setPosition = 0;
	isResetting = true;
	osMutexRelease(valveMutexId);
}

bool Outputs_get3WayValveOnResettingPosition(void)
{
	return isResetting;
}

bool Outputs_get3WayValveOnProcess(void)
{
	return currentPosition != setPosition;
}

void valveControllerTask(void *argument)
{
	VALVE_DOWN();
	osDelay(FULL_CICLE_TIME_S * osKernelGetTickFreq());
	VALVE_STOP();
	isResetting = false;
	RTOSWD_add();
	uint32_t hunredthStepTimeResoution = osKernelGetTickFreq() * FULL_CICLE_TIME_S / 100;
	for (;;)
	{
		osMutexWait(valveMutexId, portMAX_DELAY);
		if (currentPosition < setPosition)
		{
			if (valveProcess != kUp)
			{
				VALVE_STOP();
				VALVE_UP();
			}

			valveProcess = kUp;
			currentPosition++;
		}
		else if (currentPosition > setPosition)
		{
			if (valveProcess != kDown)
			{
				VALVE_STOP();
				VALVE_DOWN();
			}
			
			valveProcess = kDown;
			currentPosition--;
		}
		else
		{
			if (valveProcess != kNone)
				printf("Setting position finished: current postion = %d/100\n", currentPosition);
			
			if (isResetting)
				isResetting = false;

			valveProcess = kNone;
			VALVE_STOP();
		}

		osMutexRelease(valveMutexId);
		
		osDelay(hunredthStepTimeResoution);

		RTOSWD_update();
	}

	RTOSWD_remove();
	osThreadTerminate(NULL);		
}
