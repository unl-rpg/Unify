/********************************************************************************
 * File Name          : adc.c
 * Author             : Jack Shaver
 * Date               : 4/11/2025
 * Description        : Internal ADC Source
 ********************************************************************************/

#include "adc.h"
#include "pins.h"


#include "esp_adc/adc_oneshot.h" // testing using this only
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

//#include <string.h> //memset

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h" // command creation

#include <math.h> // log used in thermistor equations


#define NUMBER_OF_SAMPLES 3
#define BITWIDTH ADC_BITWIDTH_12
#define ATTENUATION ADC_ATTEN_DB_12


static adc_oneshot_unit_handle_t adcHandle;
static adc_channel_t adcChannel[2]; 
static adc_cali_handle_t caliHandle[2] = {NULL};

void adcInit(){
	adc_oneshot_unit_init_cfg_t unitConfig = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&unitConfig, &adcHandle);
	
	
	adcChannel[0] = PCB_TEMP_ADC_PIN - 1; // THIS ONLY WORKS ON THE S3 USING ADC 1
    adc_oneshot_chan_cfg_t config0 = {
        .atten = ATTENUATION,
        .bitwidth = BITWIDTH,
    };
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel[0], &config0));
	
	adcChannel[1] = BATTERY_VOLT_ADC_PIN - 1; // THIS ONLY WORKS ON THE S3 USING ADC 1
    adc_oneshot_chan_cfg_t config1 = {
        .atten = ATTENUATION,
        .bitwidth = BITWIDTH,
    };
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adcHandle, adcChannel[1], &config1));
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
        //ESP_ERROR_CHECK(adc_cali_raw_to_voltage(caliHandle[channel], raw, &cal)); // couldnt get this to work
        voltageAccumulate += raw;
    }
    *voltage = (uint16_t)(voltageAccumulate / sampleCount);
}

#define ADC_MAX 4095.0 // Assuming a 12-bit ADC
#define REF_VOLTAGE 3.5 // This comes from the 12db attenuation
#define RESISTOR_VALUE 10000.0 // Resistance of the 10k resistor
#define A 1.009249522e-03 // Steinhart-Hart coefficients for your thermistor
#define B 2.378405444e-04
#define C 2.019202697e-07

// Microsoft Copilot came in clutch
float calculateTemperature(uint16_t adcValue){
	// Calculate the voltage across the thermistor
    float voltage = (adcValue / ADC_MAX) * REF_VOLTAGE;

    // Calculate thermistor resistance
    float resistance = RESISTOR_VALUE * (REF_VOLTAGE / voltage - 1);

    // Steinhart-Hart equation to calculate temperature in Kelvin
    float logR = log(resistance);
    float tempKelvin = 1.0 / (A + B * logR + C * logR * logR * logR);

	// Convert to Celsius
    return tempKelvin - 273.15;
}

// Single channel read
float adcRead(adcChannelType channel){
	uint16_t reading = 0;
	float result = 0.0;
	switch(channel){
	case pcbTemperature:
		readAverage(0, NUMBER_OF_SAMPLES, &reading);
		result = calculateTemperature(reading);
		break;
	case batteryVoltage:
		readAverage(1, NUMBER_OF_SAMPLES, &reading);
		result = (reading / ADC_MAX) * REF_VOLTAGE; // do conversion
		result = result * 2.0; // there is a voltage divider on this pin
		break;
	}
	return result;
}

// ===================================== CONSOLE INTERFACE ====================================

static struct {
	struct arg_lit *battery;
	struct arg_lit *temperature;
    struct arg_end *end;
} adc_args;

static int adcDevCommand(int argc, char **argv){
	static const char* TAG = "dev-adc";
	
	int nerrors = arg_parse(argc, argv, (void  **) &adc_args);
	if (nerrors != 0) {
        arg_print_errors(stderr, adc_args.end, argv[0]);
        return 1;
    }
	
	if(adc_args.battery->count != 0){ 
		printf("Battery Voltage = %2.6f V\n", adcRead(batteryVoltage));
	}
	
	if(adc_args.temperature->count != 0){ 
		printf("PCB Temperature = %2.6f C\n", adcRead(pcbTemperature));
	}
	
	return 0;
}

void adcRegisterCommands(){
	adc_args.battery = arg_lit0("v", NULL, "Measure the Battery Voltage");
	adc_args.temperature = arg_lit0("t", NULL, "Measure the PCB Temperature");
	adc_args.end = arg_end(2);
	
	const esp_console_cmd_t cmd = {
		.command = "dev-adc",
		.help = "Manage the internal ADC.",
		.hint = NULL,
		.func = &adcDevCommand,
		.argtable = &adc_args
	};
	
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}