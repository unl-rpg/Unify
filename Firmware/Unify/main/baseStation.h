/********************************************************************************
 * File Name          : baseStation.h
 * Author             : Jack Shaver
 * Date               : 4/11/2025
 * Description        : Base Station Header
 ********************************************************************************/
 
/*
	NOTES:
		There is a short between the I2C SDA and SCL lines
		I do not have time to try to fix it more than I have already tried
		I2C should be implemented later
		For now, Espnow, Console, Blink and Buzzer should work
*/
#ifndef basestation_h
#define basestation_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

extern uint8_t espnowBaseStationMac[6];
extern uint8_t espnowTestStandMac[6];

extern void baseStationInit();

#ifdef __cplusplus
}
#endif

#endif
