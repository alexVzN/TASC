#include "RTOSWD.h"

#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#define THREAD_REFRESH_STATUS_LIST_LENGTH 24

IWDG_HandleTypeDef* iwdg;

typedef struct
{
	osThreadId_t id;
	bool isUpdated;
} ThreadRefreshStatus_t;

ThreadRefreshStatus_t list[THREAD_REFRESH_STATUS_LIST_LENGTH];

/*Definition for Mutex*/
const osMutexDef_t watchdogMutex_attribute = {
	.name = "watchdogMutex"
};

osMutexId wdMutexId;

void RTOSWD_init(IWDG_HandleTypeDef* wd)
{
	iwdg = wd;
	
	wdMutexId = osMutexNew(&watchdogMutex_attribute);
}

void RTOSWD_update(void)
{
	osThreadId_t currentId = osThreadGetId();
	bool needUpdate = false;
	osMutexWait(wdMutexId, portMAX_DELAY);
	for (int i = 0; i < THREAD_REFRESH_STATUS_LIST_LENGTH; i++)
	{
		if (list[i].id == currentId)
		{
			list[i].isUpdated = true;
		}
		
		if (list[i].id != 0 && !list[i].isUpdated)
		{
			needUpdate = true;
		}
	}
	
	if (!needUpdate)
	{
		if (HAL_IWDG_Refresh(iwdg) != HAL_OK)
		{
			Error_Handler();
		}

		for (int i = 0; i < THREAD_REFRESH_STATUS_LIST_LENGTH; i++) {
			list[i].isUpdated = false;
		}
	}

	osMutexRelease(wdMutexId);
}

void RTOSWD_add(void)
{
	osMutexWait(wdMutexId, portMAX_DELAY);
	for (int i = 0; i < THREAD_REFRESH_STATUS_LIST_LENGTH; i++)
	{
		if (list[i].id == 0)
		{
			list[i].id = osThreadGetId();
			list[i].isUpdated = false;
			osMutexRelease(wdMutexId);
			return;
		}
	}

	Error_Handler();
}

void RTOSWD_remove(void)
{
	osThreadId_t currentId = osThreadGetId();
	osMutexWait(wdMutexId, portMAX_DELAY);
	for (int i = 0; i < THREAD_REFRESH_STATUS_LIST_LENGTH; i++)
	{
		if (list[i].id == currentId)
		{
			list[i].id = 0;
			list[i].isUpdated = false;
			osMutexRelease(wdMutexId);
			return;
		}
	}
	
	Error_Handler();
}
