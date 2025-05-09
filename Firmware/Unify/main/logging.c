/********************************************************************************
 * File Name          : logging.c
 * Author             : Jack Shaver
 * Date               : 4/11/2025
 * Description        : Logging Source
 ********************************************************************************/
 
#include "logging.h"
#include "spi.h"
#include "sd.h"
#include "leds.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h" // command creation

#define MIN_FREQUENCY 1
#define MAX_FREQUENCY 100 // Could increase this by running on a timer on second core, it times out the scheduler if 1000
#define DEFAULT_FREQUENCY 100
#define MIN_DURATION 1
#define MAX_DURATION 120
#define DEFAULT_DURATION 30

static uint8_t sequencerMask = 0; // currently not used
static int frequency = DEFAULT_FREQUENCY;
static int duration = DEFAULT_DURATION;

static void loggingTask(void *arg);
SemaphoreHandle_t loggingTaskBlockSemaphore = NULL;

void loggingInit(){
	loggingTaskBlockSemaphore = xSemaphoreCreateBinary();   
	xTaskCreate(loggingTask, "loggingTask", 4096, NULL, 8, NULL);
}

logging_config_t loggingDefaultConfig(){
	logging_config_t temp = {
		.loadCell1 = true,
		.loadCell2 = false,
		.loadCell3 = false,
		.thermistor1 = false,
		.thermistor2 = false,
		.thermistor3 = false,
		.pressureTransducer1 = false,
		.pressureTransducer2 = false,
		//.frequencyHz = DEFAULT_FREQUENCY,
		.frequencyHz = 200,
		.durationSeconds = DEFAULT_DURATION 
	};
	
	return temp;
}

static uint8_t makeSequencerMask(logging_config_t cfg){
	uint8_t temp = 0;
	temp |= cfg.loadCell1;
	temp |= cfg.loadCell2 << 1;
	temp |= cfg.loadCell3 << 2;
	temp |= cfg.thermistor1 << 3;
	temp |= cfg.thermistor2 << 4;
	temp |= cfg.thermistor3 << 5;
	temp |= cfg.pressureTransducer1 << 6;
	temp |= cfg.pressureTransducer2 << 7;
	return temp;
}

void loggingConfig(logging_config_t cfg){
	sequencerMask = makeSequencerMask(cfg);
	frequency = cfg.frequencyHz;
	if(frequency < MIN_FREQUENCY){
		frequency = MIN_FREQUENCY;
	}
	if(frequency > MAX_FREQUENCY){
		frequency = MAX_FREQUENCY;
	}
	duration = cfg.durationSeconds;
	if(duration < MIN_DURATION){
		duration = MIN_DURATION;
	}
	if(duration > MAX_DURATION){
		duration = MAX_DURATION;
	}
	
	//spiAdcSetSequence(sequencerMask);
}

static bool stop = false;
static long cycles = 0;
static int delayMs = 1;
static long bufferIndex = 0;

extern void loggingStart(){
	bufferIndex = 0;
	cycles = duration * frequency;
	delayMs = (int) 1000.0 / frequency;
	stop = false;
	xSemaphoreGive(loggingTaskBlockSemaphore);
}

extern void loggingStop(){
	if(cycles != 0){
		stop = true;
	}
}

static long timestamp[30000] = {0};
static uint16_t data[30000] = {0}; // try putting this in the psram
//static bool bufferFilled = false;
void loggingTask(void *arg){

	while(1){
		if(xSemaphoreTake(loggingTaskBlockSemaphore, 0xffff) == pdTRUE){ 
			ledsSetState(ledStatus, ledFlashing); 
			while(1){
				if(stop){
					cycles = 0;
					stop = false;
					break;
				}

				
				data[bufferIndex] = spiAdcRead(0); // swap this for the faster sequencer option
				timestamp[bufferIndex] = esp_log_timestamp();
				bufferIndex++;
				
				
				cycles--;
				if(cycles <= 0){
					cycles = 0;
					break;
				}
				if(bufferIndex == 30000){
					break;
				}

				vTaskDelay(delayMs/ portTICK_PERIOD_MS);
			}
			
			sdCreateFile("log", timestamp, data, bufferIndex);
			ledsSetState(ledStatus, ledOff); 
		}
	}
}