#include "DS18B20Driver.h"

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "cmsis_os.h"
#include "task.h"

#include "Helpers/RTOSWD.h"

/* OneWire commands */
#define ONEWIRE_CMD_RSCRATCHPAD			0xBE
#define ONEWIRE_CMD_WSCRATCHPAD			0x4E
#define ONEWIRE_CMD_CPYSCRATCHPAD		0x48
#define ONEWIRE_CMD_RECEEPROM			0xB8
#define ONEWIRE_CMD_RPWRSUPPLY			0xB4
#define ONEWIRE_CMD_SEARCHROM			0xF0
#define ONEWIRE_CMD_READROM				0x33
#define ONEWIRE_CMD_MATCHROM			0x55
#define ONEWIRE_CMD_SKIPROM				0xCC

#define maxDevicesNumber				24

TIM_HandleTypeDef *tim;
const float temperatureCoef = 0.0625;
DS18B20_mode currentMode;
DS18B20_value values[maxDevicesNumber];
int devicesNumber = 0;
DS18B20_value newDevice;
bool hasNewDevice = false;

typedef struct {
	uint8_t LastDiscrepancy ; /*!< Search private */
	uint8_t LastFamilyDiscrepancy; /*!< Search private */
	uint8_t LastDeviceFlag; /*!< Search private */
	uint8_t ROM_NO[8]; /*!< 8-bytes address of last search device */
} OneWireSearchStruct_t;

void ONEWIRE_DELAY(uint16_t time_us) {
	__HAL_TIM_SET_COUNTER(tim, 0);
	uint16_t count = 0;
	while (count <= time_us) 
		count = __HAL_TIM_GET_COUNTER(tim);
}
void ONEWIRE_LOW() { DIO_GPIO_Port->BSRR = DIO_Pin << 16; }
void ONEWIRE_HIGH() { DIO_GPIO_Port->BSRR = DIO_Pin; }
void ONEWIRE_INPUT() {
	GPIO_InitTypeDef gpinit;
	gpinit.Mode = GPIO_MODE_INPUT;
	gpinit.Pull = GPIO_NOPULL;
	gpinit.Speed = GPIO_SPEED_FREQ_HIGH;
	gpinit.Pin = DIO_Pin;
	HAL_GPIO_Init(DIO_GPIO_Port, &gpinit);
}
void ONEWIRE_OUTPUT() {
	GPIO_InitTypeDef gpinit;
	gpinit.Mode = GPIO_MODE_OUTPUT_OD;
	gpinit.Pull = GPIO_NOPULL;
	gpinit.Speed = GPIO_SPEED_FREQ_HIGH;
	gpinit.Pin = DIO_Pin;
	HAL_GPIO_Init(DIO_GPIO_Port, &gpinit);
}

uint8_t OneWire_Reset()
{
	uint8_t i;
	taskENTER_CRITICAL();
	/* Line low, and wait 480us */
	ONEWIRE_LOW();
	ONEWIRE_OUTPUT();
	ONEWIRE_DELAY(480);
	ONEWIRE_DELAY(20);
	/* Release line and wait for 70us */
	ONEWIRE_INPUT();
	ONEWIRE_DELAY(70);
	/* Check bit value */
	i = HAL_GPIO_ReadPin(DIO_GPIO_Port, DIO_Pin) == GPIO_PIN_SET ? 1 : 0;
	taskEXIT_CRITICAL();
	/* Delay for 410 us */
	ONEWIRE_DELAY(410);
	/* Return value of presence pulse, 0 = OK, 1 = ERROR */
	return i;
}

void OneWire_WriteBit(uint8_t bit)
{
	if (bit) 
	{
		/* Set line low */
		taskENTER_CRITICAL();
		ONEWIRE_LOW();
		ONEWIRE_OUTPUT();
		ONEWIRE_DELAY(10);
		
		/* Bit high */
		ONEWIRE_INPUT();
		taskEXIT_CRITICAL();
		/* Wait for 55 us and release the line */
		ONEWIRE_DELAY(55);
		ONEWIRE_INPUT();
	} 
	else 
	{
		/* Set line low */
		taskENTER_CRITICAL();
		ONEWIRE_LOW();
		ONEWIRE_OUTPUT();
		ONEWIRE_DELAY(65);
		
		/* Bit high */
		ONEWIRE_INPUT();
		taskEXIT_CRITICAL();
		/* Wait for 5 us and release the line */
		ONEWIRE_DELAY(5);
		ONEWIRE_INPUT();
	}

}

uint8_t OneWire_ReadBit() 
{
	uint8_t bit = 0;
	taskENTER_CRITICAL();
	/* Line low */
	ONEWIRE_LOW();
	ONEWIRE_OUTPUT();
	ONEWIRE_DELAY(2);
	
	/* Release line */
	ONEWIRE_INPUT();
	ONEWIRE_DELAY(10);
	
	/* Read line value */
	if (HAL_GPIO_ReadPin(DIO_GPIO_Port, DIO_Pin) == GPIO_PIN_SET) {
		/* Bit is HIGH */
		bit = 1;
	}
	taskEXIT_CRITICAL();
	/* Wait 50us to complete 60us period */
	ONEWIRE_DELAY(50);
	
	/* Return bit value */
	return bit;
}

void OneWire_WriteByte(uint8_t byte) {
	uint8_t i = 8;
	/* Write 8 bits */
	while (i--) {
		/* LSB bit is first */
		OneWire_WriteBit(byte & 0x01);
		byte >>= 1;
	}
}

uint8_t OneWire_ReadByte() {
	uint8_t i = 8, byte = 0;
	while (i--) {
		byte >>= 1;
		byte |= (OneWire_ReadBit() << 7);
	}
	
	return byte;
}

uint8_t OneWire_CRC8(uint8_t *addr, uint8_t len) {
	uint8_t crc = 0, inbyte, i, mix;
	
	while (len--) {
		inbyte = *addr++;
		for (i = 8; i; i--) {
			mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) {
				crc ^= 0x8C;
			}
			inbyte >>= 1;
		}
	}
	
	/* Return calculated CRC */
	return crc;
}

uint8_t OneWire_Search(OneWireSearchStruct_t* OneWireStruct) {
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number, search_result;
	uint8_t id_bit, cmp_id_bit;
	uint8_t rom_byte_mask, search_direction;

	/* Initialize for search */
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;
	// if the last call was not the last one
	if (!OneWireStruct->LastDeviceFlag)
	{
		// 1-Wire reset
		if (OneWire_Reset(OneWireStruct)) 
		{
			/* Reset the search */
			OneWireStruct->LastDiscrepancy = 0;
			OneWireStruct->LastDeviceFlag = 0;
			OneWireStruct->LastFamilyDiscrepancy = 0;
			return 0;
		}

		// issue the search command 
		OneWire_WriteByte(ONEWIRE_CMD_SEARCHROM);  

		// loop to do the search
		do {
			// read a bit and its complement
			id_bit = OneWire_ReadBit();
			cmp_id_bit = OneWire_ReadBit();

			// check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1)) {
				break;
			}
			else {
				// all devices coupled have 0 or 1
				if (id_bit != cmp_id_bit) {
					search_direction = id_bit; // bit write value for search
				}
				else {
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < OneWireStruct->LastDiscrepancy) {
						search_direction = ((OneWireStruct->ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					}
					else {
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == OneWireStruct->LastDiscrepancy);
					}
					
					// if 0 was picked then record its position in LastZero
					if (search_direction == 0) {
						last_zero = id_bit_number;

						// check for Last discrepancy in family
						if (last_zero < 9) {
							OneWireStruct->LastFamilyDiscrepancy = last_zero;
						}
					}
				}

				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if (search_direction == 1) {
					OneWireStruct->ROM_NO[rom_byte_number] |= rom_byte_mask;
				}
				else {
					OneWireStruct->ROM_NO[rom_byte_number] &= ~rom_byte_mask;
				}
				
				// serial number search direction write bit
				OneWire_WriteBit(search_direction);

				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;

				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_byte_mask == 0) {
					//docrc8(ROM_NO[rom_byte_number]);  // accumulate the CRC
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		} while (rom_byte_number < 8);  // loop until through all ROM bytes 0-7

		// if the search was successful then
		if (!(id_bit_number < 65)) {
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			OneWireStruct->LastDiscrepancy = last_zero;

			// check for last device
			if (OneWireStruct->LastDiscrepancy == 0) {
				OneWireStruct->LastDeviceFlag = 1;
			}

			search_result = 1;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result || !OneWireStruct->ROM_NO[0]) {
		OneWireStruct->LastDiscrepancy = 0;
		OneWireStruct->LastDeviceFlag = 0;
		OneWireStruct->LastFamilyDiscrepancy = 0;
		search_result = 0;
	}

	return search_result;
}

void OneWire_Select(uint8_t* addr) {
	uint8_t i;
	OneWire_WriteByte(ONEWIRE_CMD_MATCHROM);
	
	for (i = 0; i < 8; i++) {
		OneWire_WriteByte(*(addr + i));
	}
}

int8_t OneWire_getTemperature(uint8_t* addr) {
	uint8_t i;
	OneWire_Reset();
	OneWire_Select(addr);
	OneWire_WriteByte(ONEWIRE_CMD_RSCRATCHPAD);
	
	uint8_t buff[9] = { 0 };
	
	for (i = 0; i < 9; i++) {
		buff[i] = OneWire_ReadByte();
	}
	
	int8_t value = -100;
	if (OneWire_CRC8(buff, 8) == buff[8])
	{
		float temp = (((uint16_t)buff[1] << 8) | buff[0]) * 0.0625;
		value = (int8_t)temp;
	}
	else
	{
		uint64_t address = *((uint64_t*)addr);
		if (address != 0)
		{
			printf("Device: %" PRIX64 " crc error\n", address);	
		}
	}

	return value;
}

void OneWire_startMeasurment() {
	OneWire_Reset();
	OneWire_WriteByte(0xCC);
	OneWire_WriteByte(0x44);
}

void OneWire_initDevices() {
	OneWire_Reset();
	OneWire_WriteByte(0xCC);
	OneWire_WriteByte(0x4E);
	OneWire_WriteByte(0x0);
	OneWire_WriteByte(0x0);
	OneWire_WriteByte(0x1F);
}
/*----------------------------------------------------------------------------*/

/* Definitions for readSensorsController */
osThreadId_t readSensorsControllerHandle;
const osThreadAttr_t readSensorsController_attributes = {
	.name = "readSensorsController",
	.stack_size = 128 * 4,
	.priority = (osPriority_t) osPriorityNormal,
};

void readSensorsControllerTask(void *argument);

/*Definition for Mutex*/
const osMutexDef_t readSensorsMutex_attribute = {
	.name = "readSensorsMutex"
};

osMutexId readMutexId;

void DS18B20Driver_init(TIM_HandleTypeDef* htim)
{
	tim = htim;
	DS18B20Driver_resetDevicesUID(NULL, 0);
	currentMode = kMeasurment;
	readMutexId = osMutexNew(&readSensorsMutex_attribute);
	readSensorsControllerHandle = osThreadNew(&readSensorsControllerTask, NULL, &readSensorsController_attributes);
}

void DS18B20Driver_resetDevicesUID(uint64_t* uids, uint8_t len)
{
	osMutexWait(readMutexId, portMAX_DELAY);
	memset(&values, 0, sizeof(values));
	assert_param(len <= maxDevicesNumber);
	if (uids)
	{
		for (int i = 0; i < len; i++) {
			values[i].uid = uids[i];
		}	
	}

	devicesNumber = len;
	osMutexRelease(readMutexId);
}

bool DS18B20Driver_addedNewConnectedDeviceUID(DS18B20_value* value)
{
	bool status = false;
	osMutexWait(readMutexId, portMAX_DELAY);
	if (hasNewDevice)
	{
		*value = newDevice;
		values[devicesNumber].uid = newDevice.uid;
		values[devicesNumber].temperature = newDevice.temperature;
		devicesNumber++;
		newDevice.temperature = 0;
		newDevice.uid = 0;
		hasNewDevice = false;
		status = true;
	}
	osMutexRelease(readMutexId);
	return status;
}

int DS18B20Driver_getTemperatures(DS18B20_value* value)
{
	osMutexWait(readMutexId, portMAX_DELAY);
	memcpy(value, &values, sizeof(values));
	osMutexRelease(readMutexId);
	return devicesNumber;
}

void DS18B20Driver_setMode(DS18B20_mode mode)
{
	osMutexWait(readMutexId, portMAX_DELAY);
	currentMode = mode;
	osMutexRelease(readMutexId);
}

void readSensorsControllerTask(void *argument)
{
	printf("Start measurement\n");
	RTOSWD_add();
	
	ONEWIRE_DELAY(10);
	OneWire_initDevices();
	ONEWIRE_DELAY(100);
	
	OneWireSearchStruct_t str;
	str.LastDeviceFlag = 0;
	str.LastDiscrepancy = 0;
	str.LastFamilyDiscrepancy = 0;
	OneWire_Search(&str);
	ONEWIRE_DELAY(100);

	DS18B20_mode l_mode;
	
	for (;;)
	{
		osMutexWait(readMutexId, portMAX_DELAY);
		l_mode = currentMode;
		osMutexRelease(readMutexId);
		
		if (l_mode == kSearch)
		{
			do
			{
				if (!hasNewDevice)
				{
					OneWireSearchStruct_t str;
					str.LastDeviceFlag = 0;
					str.LastDiscrepancy = 0;
					str.LastFamilyDiscrepancy = 0;
					do
					{
						OneWire_Search(&str);
						uint64_t uid = 0;
						memcpy(&uid, str.ROM_NO, 8);
						uint8_t i;
						for (i = 0; i < devicesNumber; i++)
						{
							if (values[i].uid == uid)
								break;
						}
						
						RTOSWD_update();
						
						if (i == devicesNumber)
						{
							ONEWIRE_DELAY(10);
							OneWire_startMeasurment();
		
							osDelay(1000);
		
							int8_t temp = OneWire_getTemperature((uint8_t*)(&uid));
							if (temp != -100)
							{
								osMutexWait(readMutexId, portMAX_DELAY);
								newDevice.uid = uid;
								newDevice.temperature = temp;
								hasNewDevice = true;
								osMutexRelease(readMutexId);
							} else {
								printf("Error!!!\n");
							}
						}
						
						osMutexWait(readMutexId, portMAX_DELAY);
						l_mode = currentMode;
						osMutexRelease(readMutexId);
						if (l_mode != kSearch)
							break;

					} while (!hasNewDevice);
				} else {
					osDelay(500);
					osMutexWait(readMutexId, portMAX_DELAY);
					l_mode = currentMode;
					osMutexRelease(readMutexId);
				}
				
				RTOSWD_update();
			} while (l_mode == kSearch);
			
			ONEWIRE_DELAY(10);
			OneWire_initDevices();
			ONEWIRE_DELAY(100);
		}
		
		ONEWIRE_DELAY(10);
		OneWire_startMeasurment();
		
		osDelay(1000 / portTICK_PERIOD_MS);
		
		osMutexWait(readMutexId, portMAX_DELAY);
		for (int i = 0; i < devicesNumber; i++)
			values[i].temperature = OneWire_getTemperature((uint8_t*)(&values[i].uid));
		osMutexRelease(readMutexId);
		RTOSWD_update();
		osDelay(10);
	}
	
	RTOSWD_remove();
	osThreadTerminate(osThreadGetId());
}
