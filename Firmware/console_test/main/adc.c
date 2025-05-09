/********************************************************************************
 * File Name          : adc.c
 * Author             : Jack Shaver
 * Date               : 3/24/2025
 * Description        : ADC Source
 ********************************************************************************/

#include "adc.h"

#include <stdio.h>
#include <string.h>
#include "esp_console.h"
#include "argtable3/argtable3.h" // command creation
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
// #include "esp_adc/adc_cali.h"
// #include "esp_adc/adc_cali_scheme.h"

#define NUMBER_OF_SAMPLES 3
#define BITWIDTH ADC_BITWIDTH_12
#define ATTENUATION ADC_ATTEN_DB_12

static adc_oneshot_unit_handle_t adcHandle;
static adc_channel_t adcChannel[10];

static void initADC1();
static void initChannel(uint8_t channel, gpio_num_t gpioPin);
static void readOneshot(uint8_t channel, uint16_t *voltage);
static void readAverage(uint8_t channel, uint8_t sampleCount, uint16_t* voltage);

void adcInit(gpio_num_t gpioPin){
	initADC1();
	initChannel(0, gpioPin);
}

void adcRead(uint16_t *voltage){
	//readOneshot(channel, voltage);
	readAverage(channel, 3, voltage);
}

void adcReadFloat(float *voltage){
	
}



static void initADC1(){
    adc_oneshot_unit_init_cfg_t unitConfig = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&unitConfig, &adcHandle);
}

static void initChannel(uint8_t channel, gpio_num_t gpioPin){
	adcChannel[channel] = gpioPin - 1; // this only works on the esp32s3
    adc_oneshot_chan_cfg_t config = {
        .atten = ATTENUATION,
        .bitwidth = BITWIDTH,
    };
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel[channel], &config));
}

static void readOneshot(uint8_t channel, uint16_t *voltage){
	int temp = 0;
	ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel[channel], &temp));
	*voltage = (uint16_t)temp;
}

static void readAverage(uint8_t channel, uint8_t sampleCount, uint16_t *voltage){
    int voltageAccumulate = 0;
    for (int i = 0; i < sampleCount; i++) {
        int raw, cal = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adcHandle, adcChannel[channel], &raw));
        //ESP_ERROR_CHECK(adc_cali_raw_to_voltage(caliHandle[channel], raw, &cal));
        voltageAccumulate += raw;
    }
    *voltage = (uint16_t)(voltageAccumulate / sampleCount);
}