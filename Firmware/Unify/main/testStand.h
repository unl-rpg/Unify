/********************************************************************************
 * File Name          : testStand.h
 * Author             : Jack Shaver
 * Date               : 4/11/2025
 * Description        : Test Stand Header
 ********************************************************************************/
 
/*
	NOTES:
		There is a brownout caused by high inrush current when the auxillary power rails are enabled
		This had to be locked on with a mod wire. 
		If this were fixed, standby power consumption could be lowered.
		Would have some extra over head when trying to use the SD card or the Sensors
*/
#ifndef teststand_h
#define teststand_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

extern uint8_t espnowBaseStationMac[6];
extern uint8_t espnowTestStandMac[6];

extern void testStandInit();

#ifdef __cplusplus
}
#endif

#endif
