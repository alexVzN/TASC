#include "IndicatorsDriver.h"

#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"
#include "main.h"

#include "HC595.h"
#include "Helpers/RTOSWD.h"
const int blinkCounterTimeout = 40;
const int updateDelay = 8000;
const uint8_t numbers[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
const uint8_t levelTo8[9] = { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };
const uint8_t sumE = 0x79;
const uint8_t sumR = 0x50;
const uint8_t showEmpty = 0x40;

typedef struct
{
	uint8_t value;
	IndicatorStatus status;
} NumberIndicator;

typedef struct
{
	uint8_t value;
	IndicatorStatus status;
} ProgressIndicator;

typedef struct
{
	IndicatorStatus status;
} LedIndicator;
typedef struct {
	NumberIndicator smallIndicators[7];
	NumberIndicator bigIndicators;
	ProgressIndicator progress[2];
	LedIndicator led[2];
} IndicatorsValues;

IndicatorsValues indicatorValues;

/* Definitions for shiftRegistersControllerTask */
osThreadId_t shiftRegistersControllerHandle;
const osThreadAttr_t shiftRegistersController_attributes = {
	.name = "shiftRegistersController",
	.stack_size = 256 * 4,
	.priority = (osPriority_t) osPriorityNormal,
};

void shiftRegistersControllerTask(void *argument);

/*Definition for Mutex*/
const osMutexDef_t shiftRegistersMutex_attribute = {
	.name = "shiftRegistersMutex"
};

osMutexId mutexId;

void delay(void)
{
	for (int i = 0; i < updateDelay; i++) __NOP();
	//osDelay(1);
}

void IndicatorsDriver_init()
{	
	for (int i = 0; i < 7; i++)
		indicatorValues.smallIndicators[i].status = kOff;
	indicatorValues.bigIndicators.status = kOff;
	indicatorValues.progress[0].status = kOff;
	indicatorValues.progress[1].status = kOff;
	indicatorValues.led[0].status = kOff;
	indicatorValues.led[1].status = kOff;

//	// FOR TEST : START
//	indicatorValues.smallIndicators[0].value = 9;
//	indicatorValues.smallIndicators[0].status = kStandart;
//	indicatorValues.smallIndicators[1].value = 18;
//	indicatorValues.smallIndicators[1].status = kBlink;
//	indicatorValues.smallIndicators[2].value = 27;
//	indicatorValues.smallIndicators[2].status = kError;
//	indicatorValues.smallIndicators[3].value = 36;
//	indicatorValues.smallIndicators[3].status = kLoad;
//	indicatorValues.smallIndicators[4].value = 45;
//	indicatorValues.smallIndicators[4].status = kEmpty;
//	indicatorValues.smallIndicators[5].value = 54;
//	indicatorValues.smallIndicators[5].status = kOff;
//	indicatorValues.smallIndicators[6].value = 63;
//	indicatorValues.smallIndicators[6].status = kBlink;
//	
//	indicatorValues.bigIndicators.value = 72;
//	indicatorValues.bigIndicators.status = kLoad;
//	
//	indicatorValues.led[0].status = kBlink;
//	indicatorValues.led[1].status = kBlink;
//	
//	indicatorValues.progress[0].value = 10;
//	indicatorValues.progress[0].status = kStandart;
//	
//	indicatorValues.progress[1].value = 10;
//	indicatorValues.progress[1].status = kBlink;
//	// FOR TEST : END

	mutexId = osMutexNew(&shiftRegistersMutex_attribute);
	shiftRegistersControllerHandle = osThreadNew(shiftRegistersControllerTask, NULL, &shiftRegistersController_attributes);
}

void IndicatorsDriver_setStatusForAll(IndicatorStatus status)
{
	osMutexWait(mutexId, portMAX_DELAY);
	for (int i = 0; i < 7; i++)
		indicatorValues.smallIndicators[i].status = status;
	indicatorValues.bigIndicators.status = status;
//	indicatorValues.progress[0].status = status;
	indicatorValues.progress[1].status = status;
	indicatorValues.led[0].status = status;
	indicatorValues.led[1].status = status;
	osMutexRelease(mutexId);
}

void IndicatorsDriver_setSmallNumber(uint8_t indicatorNumber, uint8_t value, IndicatorStatus status)
{
	osMutexWait(mutexId, portMAX_DELAY);
	indicatorValues.smallIndicators[indicatorNumber].value = value > 99 ? 99 : value;
	indicatorValues.smallIndicators[indicatorNumber].status = status;
	osMutexRelease(mutexId);
}

void IndicatorsDriver_setBigNumber(uint8_t value, IndicatorStatus status)
{
	osMutexWait(mutexId, portMAX_DELAY);
	indicatorValues.bigIndicators.value = value > 99 ? 99 : value;
	indicatorValues.bigIndicators.status = status;
	osMutexRelease(mutexId);
}


void IndicatorsDriver_setProgress(uint8_t indicatorNumber, uint8_t value, IndicatorStatus status)
{
	osMutexWait(mutexId, portMAX_DELAY);
	indicatorValues.progress[indicatorNumber].value = value;
	indicatorValues.progress[indicatorNumber].status = status;
	osMutexRelease(mutexId);
}


void IndicatorsDriver_setLed(uint8_t indicatorNumber, IndicatorStatus status)
{
	osMutexWait(mutexId, portMAX_DELAY);
	indicatorValues.led[indicatorNumber].status = status;
	osMutexRelease(mutexId);
}

void shiftRegistersControllerTask(void *argument)
{
	
/** internal shift registry structur:
 **	-	Reserv(2bits)
 **	-	LED(2bits)
 **	-	ProgressIndicatorCatodeControll(2bits) NPN
 **	-	ProgressIndicatorAnodes(10bits) | BigIndicatorAnodes(7bita)
 **	-	BigIndicatorCatodeControll(2bits) NPN
 **	-	SmallIndicatorAndodeControll(14bits) PNP
 **	-	Reserved(1bit)
 **	-	SmallIndicatorCatodes(7bits)
 **	*/

	RTOSWD_add();
	uint8_t blinkCounter = 0;
	bool blinkOn = true;
	bool fastBlinkOn = true;
	uint8_t showLoad = 0x01;
	
	for (;;)
	{
		uint8_t l_buff[70];
		blinkCounter++;
		if (blinkCounter == blinkCounterTimeout) {
			blinkCounter = 0;
			blinkOn = !blinkOn;
			fastBlinkOn = true;
		} else if (blinkCounter == blinkCounterTimeout / 2) {
			fastBlinkOn = false;
		}
		
		if (blinkCounter % ( blinkCounterTimeout / 5 ) == 0) {
			showLoad <<= 1;
			if (showLoad == 0b01000000)
				showLoad = 0x01;
		}

		uint8_t lockBuff[5] = { 0x00, 0x00, 0x3F, 0xFF, 0xFF};
		IndicatorsValues lockalStruct;
		osMutexWait(mutexId, portMAX_DELAY);
		memcpy(&lockalStruct, &indicatorValues, sizeof(IndicatorsValues));
		osMutexRelease(mutexId);

		for (int i = 1; i >= 0; i--) {
			bool ledStatus;
			if (lockalStruct.led[i].status == kStandart)
				ledStatus = true;
			else if (lockalStruct.led[i].status == kBlink)
				ledStatus = blinkOn;
			else
				ledStatus = false;

			if (ledStatus)
				lockBuff[0] |= i == 1 ? 0b00100000 : 0b00010000;
		}
		
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[0].status;
			uint8_t smallValue = sStatus == kError ? sumR : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[0].value % 10];
			IndicatorStatus bStatus = lockalStruct.bigIndicators.status;
			uint8_t bigValue = bStatus == kError ? sumR : bStatus == kEmpty ? showEmpty : bStatus == kLoad ? showLoad : numbers[lockalStruct.bigIndicators.value % 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[3] = 0b11111110;
			lockBuff[2] = 0x7F;
			lockBuff[1] = (bStatus == kBlink && !blinkOn) || (bStatus == kFastBlink && !fastBlinkOn)  || bStatus == kOff ? 0x00 : bigValue;
			memcpy(l_buff, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[0].status;
			uint8_t smallValue = sStatus == kError ? sumE : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[0].value / 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn)  || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[3] = 0b11111101;
			memcpy(l_buff + 5, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[1].status;
			uint8_t smallValue = sStatus == kError ? sumR : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[1].value % 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn)  || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[3] = 0b11111011;
			memcpy(l_buff + 10, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[1].status;
			uint8_t smallValue = sStatus == kError ? sumE : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[1].value / 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn)  || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[3] = 0b11110111;
			memcpy(l_buff + 15, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[2].status;
			uint8_t smallValue = sStatus == kError ? sumR : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[2].value % 10];
			IndicatorStatus bStatus = lockalStruct.bigIndicators.status;
			uint8_t bigValue = bStatus == kError ? sumE : bStatus == kEmpty ? showEmpty : bStatus == kLoad ? showLoad : numbers[lockalStruct.bigIndicators.value / 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn)  || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[3] = 0b11101111;
			lockBuff[2] = 0xBF;
			lockBuff[1] = (bStatus == kBlink && !blinkOn) || (bStatus == kFastBlink && !fastBlinkOn) || bStatus == kOff ? 0x00 : bigValue;
			memcpy(l_buff + 20, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[2].status;
			uint8_t smallValue = sStatus == kError ? sumE : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[2].value / 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[3] = 0b11011111;
			memcpy(l_buff + 25, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[3].status;
			uint8_t smallValue = sStatus == kError ? sumR : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[3].value % 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[3] = 0b10111111;
			memcpy(l_buff + 30, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[3].status;
			uint8_t smallValue = sStatus == kError ? sumE : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[3].value / 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[3] = 0b01111111;
			memcpy(l_buff + 35, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[4].status;
			uint8_t smallValue = sStatus == kError ? sumR : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[4].value % 10];
			IndicatorStatus bStatus = lockalStruct.progress[0].status;
			uint8_t bValue = lockalStruct.progress[0].value;
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[3] = 0xFF;
			lockBuff[2] = 0b00111110;
			lockBuff[1] = (bStatus == kBlink && !blinkOn) || (bStatus == kFastBlink && !fastBlinkOn) || bStatus == kOff ? 0x00 : bValue < 8 ? levelTo8[bValue % 8] : 0xFF;
			lockBuff[0] |= (bStatus == kBlink && !blinkOn) || (bStatus == kFastBlink && !fastBlinkOn) || bStatus == kOff ? 0x00 : (bValue <= 8 ? 0b0100 : bValue == 9 ? 0b0101 : 0b0111);
			memcpy(l_buff + 40, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[4].status;
			uint8_t smallValue = sStatus == kError ? sumE : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[4].value / 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[2] = 0b00111101;
			memcpy(l_buff + 45, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[5].status;
			uint8_t smallValue = sStatus == kError ? sumR : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[5].value % 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[2] = 0b00111011;
			memcpy(l_buff + 50, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[5].status;
			uint8_t smallValue = sStatus == kError ? sumE : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[5].value / 10];
			IndicatorStatus bStatus = lockalStruct.progress[1].status;
			uint8_t bValue = lockalStruct.progress[1].value;
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[2] = 0b00110111;
			lockBuff[0] &= 0xF0;
			lockBuff[1] = (bStatus == kBlink && !blinkOn) || (bStatus == kFastBlink && !fastBlinkOn) || bStatus == kOff ? 0x00 : bValue < 8 ? levelTo8[bValue % 8] : 0xFF;
			lockBuff[0] |= (bStatus == kBlink && !blinkOn) || (bStatus == kFastBlink && !fastBlinkOn) || bStatus == kOff ? 0x00 : (bValue <= 8 ? 0b1000 : bValue == 9 ? 0b1001 : 0b1011);
			memcpy(l_buff + 55, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[6].status;
			uint8_t smallValue = sStatus == kError ? sumR : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[6].value % 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[2] = 0b00101111;
			memcpy(l_buff + 60, lockBuff, sizeof(lockBuff));
		}
		{
			IndicatorStatus sStatus = lockalStruct.smallIndicators[6].status;
			uint8_t smallValue = sStatus == kError ? sumE : sStatus == kEmpty ? showEmpty : sStatus == kLoad ? showLoad : numbers[lockalStruct.smallIndicators[6].value / 10];
			lockBuff[4] = ~((sStatus == kBlink && !blinkOn) || (sStatus == kFastBlink && !fastBlinkOn) || sStatus == kOff ? 0x00 : smallValue);
			lockBuff[2] = 0b00011111;
			memcpy(l_buff + 65, lockBuff, sizeof(lockBuff));
		}

		HC595_sendBuff(l_buff, sizeof(l_buff), 5);
		osDelay(20);
		RTOSWD_update();
	}

	RTOSWD_remove();
	osThreadTerminate(osThreadGetId());
}

