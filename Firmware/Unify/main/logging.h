/********************************************************************************
 * File Name          : logging.h
 * Author             : Jack Shaver
 * Date               : 4/11/2025
 * Description        : Logging Header
 ********************************************************************************/
 
#ifndef logging_h
#define logging_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct {
    bool loadCell1;
	bool loadCell2;
	bool loadCell3;
	bool thermistor1;
	bool thermistor2;
	bool thermistor3;
	bool pressureTransducer1;
	bool pressureTransducer2;
    int frequencyHz;
	int durationSeconds;
} logging_config_t;

extern void loggingInit();

extern logging_config_t loggingDefaultConfig();
extern void loggingConfig(logging_config_t cfg);

extern void loggingStart();
extern void loggingStop();


#ifdef __cplusplus
}
#endif

#endif