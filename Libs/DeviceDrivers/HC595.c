#include "HC595.h"

#include <string.h>

#include "cmsis_os.h"
#include "main.h"

#include "Helpers/RTOSWD.h"

uint8_t sendSpiBuff[100];
int buffSize;
int number = 0;

SPI_HandleTypeDef* spi = NULL;

osThreadId_t sendSPIHandle;
const osThreadAttr_t sendSPI_attributes = {
	.name = "sendSPIController",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityNormal,
};

void sendSPITask(void *argument);

/*Definition for Mutex*/
const osMutexDef_t sendSPIMutex_attribute = {
	.name = "sendSPIMutex"
};

osMutexId sendSPIMutexId;

void sendBuff(uint8_t* buff)
{
	HAL_GPIO_WritePin(SPI_NOE_GPIO_Port, SPI_NOE_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SPI_NMR_GPIO_Port, SPI_NMR_Pin, GPIO_PIN_RESET);
	for (int i = 0; i < 10; i++) ;
	HAL_GPIO_WritePin(SPI_NMR_GPIO_Port, SPI_NMR_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SPI_NSTCP_GPIO_Port, SPI_NSTCP_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(spi, buff, number, 20);
	for (int i = 0; i < 10; i++) ;
	HAL_GPIO_WritePin(SPI_NSTCP_GPIO_Port, SPI_NSTCP_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(SPI_NOE_GPIO_Port, SPI_NOE_Pin, GPIO_PIN_RESET);
}

void HC595_init(SPI_HandleTypeDef* hspi)
{
	spi = hspi;
	sendSPIMutexId = osMutexNew(&sendSPIMutex_attribute);
	sendSPIHandle = osThreadNew(sendSPITask, NULL, &sendSPI_attributes);
}

void HC595_sendBuff(uint8_t* buff, int length, int numbOfDevices)
{
	osMutexWait(sendSPIMutexId, portMAX_DELAY);
	buffSize = length;
	memcpy(sendSpiBuff, buff, buffSize);
	number = numbOfDevices;
	osMutexRelease(sendSPIMutexId);
}

void sendSPITask(void *argument)
{
	RTOSWD_add();
	uint8_t l_buff[100];
	for (;;)
	{
		osMutexWait(sendSPIMutexId, portMAX_DELAY);
		memcpy(l_buff, sendSpiBuff, buffSize);
		osMutexRelease(sendSPIMutexId);
		for (int i = 0; i < buffSize / number; i++)
		{
			sendBuff(l_buff + i * number);
			osDelay(1);
		}

		RTOSWD_update();
	}

	RTOSWD_remove();
	osThreadTerminate(NULL);
}