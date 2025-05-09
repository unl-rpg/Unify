/********************************************************************************
 * File Name          : sd.h
 * Author             : Jack Shaver
 * Date               : 4/11/2025
 * Description        : SD Card SDMMC header
 ********************************************************************************/
 
 
/*
	NOTES:
		Just like the SPI and I2C drivers, this encorperates higher level datalogging functionallity
		Someone with more time than me should go back and split this up
			Have an SD file which just initializes the driver, creates files and writes to them
			Have a logging file which takes in data and routes it appropriatly into the SD file
*/
#ifndef sd_h
#define sd_h

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

extern void sdInit();

extern void sdCreateFile(char *filename, long *timeStamp, uint16_t *data, long samples);

#ifdef __cplusplus
}
#endif

#endif
