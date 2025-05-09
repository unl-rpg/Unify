/********************************************************************************
 * File Name          : buzzer.c
 * Author             : Jack Shaver
 * Date               : 4/1/2025
 * Description        : Buzzer Source
 ********************************************************************************/

#include "buzzer.h"

#include <stdio.h>
#include <string.h>
#include "esp_console.h"
#include "argtable3/argtable3.h" // command creation
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define MIN_BUZZER_PERIOD 50
#define MAX_BUZZER_PERIOD 5000
#define MIN_BUZZER_DUTY_CYCLE 0
#define MAX_BUZZER_DUTY_CYCLE 100

typedef struct{
    gpio_num_t pin;
	bool enable;
	int period;
	int dutyCycle;
} buzzerStruct;
static buzzerStruct buzzerConfig;

static struct {
    struct arg_int *enable;
    struct arg_int *period;
	struct arg_int *duty;
	struct arg_lit *query;
    struct arg_end *end;
} buzzer_args;

static void buzzerTask(void *arg);
static int buzzerCommand(int argc, char **argv);

void buzzerInit(gpio_num_t pin){
	buzzerStruct config = {
		.pin = pin,
		.enable = false,
		.period = 1000,
		.dutyCycle = 10
	};
	buzzerConfig = config;
	
	ESP_ERROR_CHECK(gpio_reset_pin(buzzerConfig.pin));
    ESP_ERROR_CHECK(gpio_set_direction(buzzerConfig.pin, GPIO_MODE_OUTPUT));
	xTaskCreate(buzzerTask, "buzzerTask", 4096, NULL, 1, NULL);
}

bool buzzerGetEnable(){
	return buzzerConfig.enable;
}

int buzzerGetPeriod(){
	return buzzerConfig.period;
}

int buzzerGetDutyCycle(){
	return buzzerConfig.dutyCycle;
}

void buzzerSetEnable(bool enable){
	buzzerConfig.enable = enable;
}

void buzzerSetPeriod(int period){
	if(period < MIN_BUZZER_PERIOD) period = MIN_BUZZER_PERIOD;
	if(period > MAX_BUZZER_PERIOD) period = MAX_BUZZER_PERIOD;
	buzzerConfig.period = period;
}

void buzzerSetDutyCycle(int dutyCycle){
	if(dutyCycle < MIN_BUZZER_DUTY_CYCLE) dutyCycle = MIN_BUZZER_DUTY_CYCLE;
	if(dutyCycle > MAX_BUZZER_DUTY_CYCLE) dutyCycle = MAX_BUZZER_DUTY_CYCLE;
	buzzerConfig.dutyCycle = dutyCycle;
}

void buzzerRegisterCommands(){
	buzzer_args.enable = arg_int0("e", NULL, "<0|1>", "Enable or Disable the buzzing");
	buzzer_args.period = arg_int0("p", NULL, "<50-5000>", "Buzzing period in ms");
	buzzer_args.duty = arg_int0("d", NULL, "<0-100>", "Buzzing Duty Cycle as a percent");
	buzzer_args.query = arg_lit0("q", NULL, "Query the buzzer parameters");
	buzzer_args.end = arg_end(2);
	
	const esp_console_cmd_t cmd = {
		.command = "dev-buzzer",
		.help = "Manage the buzzer.",
		.hint = NULL,
		.func = &buzzerCommand,
		.argtable = &buzzer_args
	};
	
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

// Private Functions

static void buzzerTask(void *arg){	
	static bool buzzerState = 0;
	static bool disableEdge = 1;
    while(1){
		buzzerState = !buzzerState;
		if(buzzerConfig.enable == true){
			ESP_ERROR_CHECK(gpio_set_level(buzzerConfig.pin, buzzerState));
			disableEdge = 1;
		}else{
			if(disableEdge == 1){
				disableEdge = 0;
				ESP_ERROR_CHECK(gpio_set_level(buzzerConfig.pin, 0));
			}
		}
		int sleepFor;
		if(buzzerState == false){
			sleepFor = (int) buzzerConfig.period * ((100 - buzzerConfig.dutyCycle) / 100.0);
		}else{
			sleepFor = (int) buzzerConfig.period * (buzzerConfig.dutyCycle / 100.0);
		}

		//printf("sleepFor: %d\n", sleepFor);
		vTaskDelay(sleepFor / portTICK_PERIOD_MS);
        //vTaskDelay((buzzerConfig.period / 2) / portTICK_PERIOD_MS);
    }
}

static int buzzerCommand(int argc, char **argv){
	static const char* TAG = "dev-buzzer";
	
	int nerrors = arg_parse(argc, argv, (void  **) &buzzer_args);
	if (nerrors != 0) {
        arg_print_errors(stderr, buzzer_args.end, argv[0]);
        return 1;
    }
	if(buzzer_args.enable->count != 0){
		buzzerConfig.enable = buzzer_args.enable->ival[0];
	}
	if(buzzer_args.period->count != 0){
		int period = buzzer_args.period->ival[0];
		if(period < MIN_BUZZER_PERIOD) period = MIN_BUZZER_PERIOD;
		if(period > MAX_BUZZER_PERIOD) period = MAX_BUZZER_PERIOD;
		buzzerConfig.period = period;
	}
	if(buzzer_args.duty->count != 0){
		int dutyCycle = buzzer_args.duty->ival[0];
		if(dutyCycle < MIN_BUZZER_DUTY_CYCLE) dutyCycle = MIN_BUZZER_DUTY_CYCLE;
		if(dutyCycle > MAX_BUZZER_DUTY_CYCLE) dutyCycle = MAX_BUZZER_DUTY_CYCLE;
		buzzerConfig.dutyCycle = dutyCycle;
	}
	if(buzzer_args.query->count != 0){
		printf("enable: %d\n", buzzerConfig.enable);
		printf("period: %d\n", buzzerConfig.period);
		printf("dutycycle: %d\n", buzzerConfig.dutyCycle);
	}
	
	return 0;
}