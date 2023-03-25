#ifndef __INDICATOR_DRIVERS_H
#define __INDICATOR_DRIVERS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

	typedef enum
	{
		kStandart = 0,	// Number | Progress | LED
		kBlink,			// Number | Progress | LED
		kFastBlink,		// Number | Progress | LED
		kError,			// Number
		kBlinkError,    // Number
		kLoad,			// Number
		kEmpty,			// Number
		kOff			// Number | Progress | LED
	} IndicatorStatus;
	
	void IndicatorsDriver_init();
	void IndicatorsDriver_setStatusForAll(IndicatorStatus status);
	void IndicatorsDriver_setSmallNumber(uint8_t indicatorNumber, uint8_t value, IndicatorStatus status);
	void IndicatorsDriver_setBigNumber(uint8_t value, IndicatorStatus status);
	void IndicatorsDriver_setProgress(uint8_t indicatorNumber, uint8_t value, IndicatorStatus status);
	void IndicatorsDriver_setLed(uint8_t indicatorNumber, IndicatorStatus status);

#ifdef __cplusplus
}
#endif

#endif /* __INDICATOR_DRIVERS_H */