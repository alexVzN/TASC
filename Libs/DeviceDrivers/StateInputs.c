#include "StateInputs.h"

#include "main.h"
#include "cmsis_os.h"

#define LONG_PRESS_MAX_COUNT_S	3

TIM_HandleTypeDef* encoderTim;
int longPressCounter = 0;
int longMainPressCounter = 0;

bool isMatchMode = false;

bool mainBtnPress = false;
bool matchBtnPress = false;

void StateInputs_init(TIM_HandleTypeDef* htim)
{
	encoderTim = htim;
	HAL_TIM_Encoder_Start(encoderTim, TIM_CHANNEL_1 | TIM_CHANNEL_2);
}
	
int StateInputs_getEncoderValue(void)
{
	int value = -(int16_t)__HAL_TIM_GET_COUNTER(encoderTim);
	__HAL_TIM_SET_COUNTER(encoderTim, 0);
	return value;
}

bool StateInputs_isMatchMode(void)
{
	if (!matchBtnPress && longPressCounter != 0)
	{
		if (osKernelGetSysTimerCount() - longPressCounter > LONG_PRESS_MAX_COUNT_S * osKernelGetSysTimerFreq())
		{
			longPressCounter = 0;
			isMatchMode = !isMatchMode;
		}
	}
	
	return isMatchMode;
}

bool StateInputs_switchMatchTarget(void)
{
	if (!matchBtnPress && longPressCounter != 0)
	{
		if (osKernelGetSysTimerCount() - longPressCounter < LONG_PRESS_MAX_COUNT_S * osKernelGetSysTimerFreq())
		{
			longPressCounter = 0;
			return isMatchMode;
		}
	}

	return false;
}

bool StateInputs_onClickedMainButton(void)
{
	if (!mainBtnPress && longMainPressCounter != 0)
	{
		if (osKernelGetSysTimerCount() - longMainPressCounter < LONG_PRESS_MAX_COUNT_S * osKernelGetSysTimerFreq())
		{
			longMainPressCounter = 0;
			return true;
		}
	}

	return false;
}

bool StateInputs_termostateStatus(void)
{
	return HAL_GPIO_ReadPin(Termostate_status_GPIO_Port, Termostate_status_Pin) == GPIO_PIN_RESET;
}

bool StateInputs_resetDevice(void)
{
	bool returnedStatus = false;
	if (!mainBtnPress && longMainPressCounter != 0)
	{
		if (osKernelGetSysTimerCount() - longMainPressCounter > LONG_PRESS_MAX_COUNT_S * osKernelGetSysTimerFreq())
		{
			longMainPressCounter = 0;
			returnedStatus = matchBtnPress;
		}
	}
	
	return returnedStatus;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == Encoder_SW_Pin)
	{
		mainBtnPress = HAL_GPIO_ReadPin(Encoder_SW_GPIO_Port, Encoder_SW_Pin) == GPIO_PIN_RESET;
		if (mainBtnPress)
		{
			longMainPressCounter = osKernelGetSysTimerCount();
		}	
	}
	else if (GPIO_Pin == Prgm__sensor_btn_Pin)
	{
		matchBtnPress = HAL_GPIO_ReadPin(Prgm__sensor_btn_GPIO_Port, Prgm__sensor_btn_Pin) == GPIO_PIN_RESET;
		if (matchBtnPress) {
			longPressCounter = osKernelGetSysTimerCount();
		}	
	}
}