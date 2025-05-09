/********************************************************************************
 * File Name          : leds.c
 * Author             : Jack Shaver
 * Date               : 4/7/2025
 * Description        : Runs the Power, Charge and Armed LEDs
 ********************************************************************************/
 
#include "leds.h"
 
#include <stdio.h>
#include <string.h>
#include "esp_console.h"
#include "argtable3/argtable3.h" // command creation
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <stdbool.h>

#include "i2c.h"

#define MIN_BLINK_PERIOD 50
#define MAX_BLINK_PERIOD 5000

typedef struct{
	ledsStateType state;
	int period;
} ledStruct;
static ledStruct ledConfigs[3]; // Power, Charge, Status

//typedef struct{
//	bool enable;
//	int error;
//}errorStruct;
//static errorStruct errorConfig;

static struct {
    struct arg_lit *power;
	struct arg_lit *charge;
	struct arg_lit *status;
    struct arg_int *period;
	struct arg_int *state;
	struct arg_lit *query;
    struct arg_end *end;
} leds_args;

static void powerLedTask(void *arg);
static void chargeLedTask(void *arg);
static void statusLedTask(void *arg);
//static void errorLedTask(void *arg);
static int ledsCommand(int argc, char **argv);

void ledsInit(){
	ledStruct powerLedConfig = {
		.state = ledFlashing,
		.period = 2000,
	};
	
	ledStruct chargeLedConfig = {
		.state = ledOff,
		.period = 1000,
	};

	ledStruct statusLedConfig = {
		.state = ledOff,
		.period = 500,
	};
	
	//errorStruct errorLedConfig = {
	//	.enable = false,
	//	.error = 0x00,
	//};
	
	i2cSetGpioSignal(I2C_ERROR_LED, false);
	
	ledConfigs[ledPower] = powerLedConfig;
	ledConfigs[ledCharge] = chargeLedConfig;
	ledConfigs[ledStatus] = statusLedConfig;
	//errorConfig = errorLedConfig;
	
	xTaskCreate(powerLedTask, "powerLedTask", 4096, NULL, 1, NULL);
	xTaskCreate(chargeLedTask, "chargeLedTask", 4096, NULL, 1, NULL);
	xTaskCreate(statusLedTask, "statusLedTask", 4096, NULL, 1, NULL);
	//xTaskCreate(errorLedTask, "errorLedTask", 4096, NULL, 1, NULL);
}

void ledsSetState(ledsType led, ledsStateType state){
	ledConfigs[led].state = state;
}

void ledsSetPeriod(ledsType led, int period){
	if(period < MIN_BLINK_PERIOD) period = ledConfigs[led].period;
	if(period > MAX_BLINK_PERIOD) period = ledConfigs[led].period;
	ledConfigs[led].period = period;
}

ledsStateType ledsGetState(ledsType led){
	return ledConfigs[led].state;
}

int ledsGetPeriod(ledsType led){
	return ledConfigs[led].period;
}

// Change these later to print out bcd flashes
void ledsReportError(int error){
	//errorConfig.error = error;
	//errorConfig.enable = true;
	i2cSetGpioSignal(I2C_ERROR_LED, true);
}

void ledsClearError(){
	//errorConfig.enable = false;
	i2cSetGpioSignal(I2C_ERROR_LED, false);
}


void ledsRegisterCommands(){
	leds_args.power = arg_lit0("p", NULL, "Select the power LED");
	leds_args.charge = arg_lit0("c", NULL, "Select the charge LED");
	leds_args.status = arg_lit0("s", NULL, "Select the status LED");
	leds_args.state = arg_int0("e", NULL, "<0|1", "Set the LED on or off");
	leds_args.period = arg_int0("t", NULL, "<50-5000>", "Set the LED to blink with period.");
	leds_args.query = arg_lit0("q", NULL, "Query the selected LED");
	leds_args.end = arg_end(2);
	
	const esp_console_cmd_t cmd = {
		.command = "dev-leds",
		.help = "Manage the basic front panel LEDs.",
		.hint = NULL,
		.func = &ledsCommand,
		.argtable = &leds_args
	};
	
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

int ledsCommand(int argc, char **argv){
	static const char* TAG = "dev-leds";
	
	int nerrors = arg_parse(argc, argv, (void  **) &leds_args);
	if (nerrors != 0) {
        arg_print_errors(stderr, leds_args.end, argv[0]);
        return 1;
    }
	ledsType ledSelection;
	bool ledIsSelected = 0;
	if(leds_args.power->count != 0){
		ledSelection = ledPower;
		ledIsSelected = 1;
	}
	if(leds_args.charge->count != 0){
		ledSelection = ledCharge;
		ledIsSelected = 1;
	}
	if(leds_args.status->count != 0){
		ledSelection = ledStatus;
		ledIsSelected = 1;
	}
	
	if(!ledIsSelected){
		printf("Must Select an LED using -p, -c or -a\n");
		return 1;
	}
	
	if(leds_args.state->count != 0){
		int state = leds_args.state->ival[0];
		if(state == 0){
			ledConfigs[ledSelection].state = ledOff;
		}else if(state == 1){
			ledConfigs[ledSelection].state = ledOn;
		}else{
			printf("Invalid LED state input. Must be 0 or 1.\n");
			return 1;
		}
	}
	
	if(leds_args.period->count != 0){
		int period = leds_args.period->ival[0];
		if(period < MIN_BLINK_PERIOD) period = ledConfigs[ledSelection].period;
		if(period > MAX_BLINK_PERIOD) period = ledConfigs[ledSelection].period;
		ledConfigs[ledSelection].state = ledFlashing;
		ledConfigs[ledSelection].period = period;
	}
	
	if(leds_args.query->count != 0){
		switch(ledConfigs[ledSelection].state){
		case ledOn:
			printf("state: on\n");
			break;
		case ledOff:
			printf("state: off\n");
			break;
		case ledFlashing:
			printf("state: flashing\n");
			break;
		}
		
		printf("period: %d\n", ledConfigs[ledSelection].period);
	}
	
	return 0;
}

void powerLedTask(void *arg){
	static bool blinkState = 0;
	
	while(1){
		switch(ledConfigs[ledPower].state){
		case ledOn:
			blinkState = true;
			break;
		case ledOff:
			blinkState = false;
			break;
		case ledFlashing:
			blinkState = !blinkState;
			break;
		}
		
		i2cSetGpioSignal(I2C_POWER_LED, blinkState);
		vTaskDelay((ledConfigs[ledPower].period / 2) / portTICK_PERIOD_MS);
	}
}


void chargeLedTask(void *arg){
	static bool blinkState = 0;
	
	while(1){
		switch(ledConfigs[ledCharge].state){
		case ledOn:
			blinkState = true;
			break;
		case ledOff:
			blinkState = false;
			break;
		case ledFlashing:
			blinkState = !blinkState;
			break;
		}
		
		i2cSetGpioSignal(I2C_CHARGE_LED, blinkState);
		vTaskDelay((ledConfigs[ledCharge].period / 2) / portTICK_PERIOD_MS);
	}
}


void statusLedTask(void *arg){
	static bool blinkState = 0;
	
	while(1){
		switch(ledConfigs[ledStatus].state){
		case ledOn:
			blinkState = true;
			break;
		case ledOff:
			blinkState = false;
			break;
		case ledFlashing:
			blinkState = !blinkState;
			break;
		}
		
		i2cSetGpioSignal(I2C_STATUS_LED, blinkState);
		vTaskDelay((ledConfigs[ledStatus].period / 2) / portTICK_PERIOD_MS);
	}
}

/*
void errorLedTask(void *arg){
	static bool output = 0;
	static uint8_t digit = 1;
	
	while(1){
		if(errorConfig.enable == true){
			output = errorConfig.error & bit;
		}else{
			output = 0;
		}
		
		digit--;
		if(digit < 0){ 
			digit = 1;
		}
		
		i2cSetGpioSignal(I2C_STATUS_LED, output);
		vTaskDelay((1000 / 2) / portTICK_PERIOD_MS);
	}
}
*/