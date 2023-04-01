#include "MemoryManagement.h"

#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"
#include "task.h"

#define CUSTOM_MEMORY_PAGE_ADD 0x0800FC00

#define MINIMAL_TEMPERATURE_MIN 20
#define MINIMAL_TEMPERATURE_MAX 60
#define PIPE_TEMPERATURE_MIN 20
#define PIPE_TEMPERATURE_MAX 80

#pragma pack(push, 1)
typedef struct {
	uint64_t uids[9];
	int8_t minimalTemperature;
	int8_t pipeTemperature;
} MemoryStruct_t;
#pragma pack(pop)

MemoryStruct_t customMemory;

void MM_init(void)
{
	memcpy(&customMemory, (void*)CUSTOM_MEMORY_PAGE_ADD, sizeof(customMemory));
	if (customMemory.minimalTemperature < MINIMAL_TEMPERATURE_MIN || customMemory.minimalTemperature > MINIMAL_TEMPERATURE_MAX) {
		customMemory.minimalTemperature = 40;
	}
	
	if (customMemory.pipeTemperature < PIPE_TEMPERATURE_MIN || customMemory.pipeTemperature > PIPE_TEMPERATURE_MAX) {
		customMemory.pipeTemperature = 40;
	}
}
	
uint64_t* MM_getUids(void)
{
	return customMemory.uids;
}

void MM_setUids(uint64_t* uids)
{
	memcpy(customMemory.uids, uids, sizeof(uint64_t) * 9);
	MM_updateFlash();
}

int8_t MM_getMinimalTankTemperature(void)
{
	return customMemory.minimalTemperature;
}

void MM_setMinimalTankTemperature(int8_t temperature)
{
	if (temperature >= MINIMAL_TEMPERATURE_MIN && temperature <= MINIMAL_TEMPERATURE_MAX) {
		customMemory.minimalTemperature = temperature;
	} else if (temperature < MINIMAL_TEMPERATURE_MIN) {
		customMemory.minimalTemperature = MINIMAL_TEMPERATURE_MIN;
	} else if (temperature > MINIMAL_TEMPERATURE_MAX) {
		customMemory.minimalTemperature = MINIMAL_TEMPERATURE_MAX;
	}
}

int8_t MM_getPipeTemperature(void)
{
	return customMemory.pipeTemperature;
}

void MM_setPipeTemperature(int8_t temperature)
{
	if (temperature >= PIPE_TEMPERATURE_MIN && temperature <= PIPE_TEMPERATURE_MAX) {
		customMemory.pipeTemperature = temperature;	
	} else if (temperature < PIPE_TEMPERATURE_MIN) {
		customMemory.pipeTemperature = PIPE_TEMPERATURE_MIN;
	} else if (temperature > PIPE_TEMPERATURE_MAX) {
		customMemory.pipeTemperature = PIPE_TEMPERATURE_MAX;
	}
}

void MM_reset(void)
{
	memset(customMemory.uids, 0, sizeof(uint64_t) * 9);
	customMemory.minimalTemperature = 40;
	customMemory.pipeTemperature = 40;
	MM_updateFlash();
}

void MM_updateFlash(void)
{
	if (memcmp(&customMemory, (void*)CUSTOM_MEMORY_PAGE_ADD, sizeof(customMemory)) == 0) {
		return;
	}

	FLASH_EraseInitTypeDef str = {
		.TypeErase = FLASH_TYPEERASE_PAGES,
		.Banks = FLASH_BANK_1,
		.PageAddress = CUSTOM_MEMORY_PAGE_ADD,
		.NbPages = 1
	};

	taskENTER_CRITICAL();
	HAL_FLASH_Unlock();
	uint32_t PAGEError;
	HAL_FLASHEx_Erase(&str, &PAGEError);
	int shiftAddress = 0;
	int shiftBuffer = 0;
	while (shiftAddress <= sizeof(customMemory))
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, CUSTOM_MEMORY_PAGE_ADD + shiftAddress, *(((uint32_t*)&customMemory) + shiftBuffer));
		shiftAddress += 4;
		shiftBuffer += 1;
	}

	HAL_FLASH_Unlock();
	taskEXIT_CRITICAL();
}
