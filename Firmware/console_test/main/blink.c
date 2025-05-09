/********************************************************************************
 * File Name          : blink.c
 * Author             : Jack Shaver
 * Date               : 3/24/2025
 * Description        : Blink Source
 ********************************************************************************/

#include "blink.h"

#include <stdio.h>
#include <string.h>
#include "esp_console.h"
#include "argtable3/argtable3.h" // command creation
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define MIN_BLINK_PERIOD 50
#define MAX_BLINK_PERIOD 10000

typedef struct{
    gpio_num_t pin;
	bool enable;
	int period;
} blinkStruct;
static blinkStruct resources[2];

static struct {
	struct arg_int *led;
    struct arg_int *enable;
    struct arg_int *period;
	struct arg_lit *view;
    struct arg_end *end;
} blink_args_1;

static struct {
	struct arg_str *name;
	struct arg_str *param;
	struct arg_str *value;
	struct arg_end *end;
} blink_args_2;

static void blinkTask0(void *arg);
static void blinkTask1(void *arg);
static int blinkCommand1(int argc, char **argv);
static int blinkCommand2(int argc, char **argv);

void blinkInit(gpio_num_t led0, gpio_num_t led1){
	blinkStruct blinkConfig = {
		.pin = led0,
		.enable = true,
		.period = 1000,
	};
	resources[0] = blinkConfig;
	
	blinkConfig.pin = led1;
	blinkConfig.enable = false;
	resources[1] = blinkConfig;
	
	xTaskCreate(blinkTask0, "blinkTask1", 4096, NULL, 1, NULL);
	xTaskCreate(blinkTask1, "blinkTask2", 4096, NULL, 1, NULL);
}

int blinkGetEnable(int led, bool* enable){
	if(led < 0 || led > 2){
		ESP_LOGE("getEnable", "Invalid LED selection. Must be 0 or 1.");
		return 1;
	}
	*enable = resources[led].enable;
	
	return 0;
}

int blinkGetPeriod(int led, int* period){
	if(led < 0 || led > 2){
		ESP_LOGE("getPeriod", "Invalid LED selection. Must be 0 or 1.");
		return 1;
	}
	*period = resources[led].period;
	
	return 0;
}

int blinkSetEnable(int led, bool enable){
	if(led < 0 || led > 2){
		ESP_LOGE("setEnable", "Invalid LED selection. Must be 0 or 1.");
		return 1;
	}
	resources[led].enable = enable;
	
	return 0;
}

int blinkSetPeriod(int led, int period){
	if(led < 0 || led > 2){
		ESP_LOGE("setPeriod", "Invalid LED selection. Must be 0 or 1.");
		return 1;
	}

	if(period < MIN_BLINK_PERIOD) period = MIN_BLINK_PERIOD;
	if(period > MAX_BLINK_PERIOD) period = MAX_BLINK_PERIOD;
	resources[led].period = period;
	
	return 0;
}

void blinkRegisterCommands(){
	blink_args_1.led = arg_int1(NULL, NULL, "<0|1>", "LED to address");
	blink_args_1.enable = arg_int0("e", NULL, "<0|1>", "Enable or Disable the blinking");
	blink_args_1.period = arg_int0("p", NULL, NULL, "Blinking period in ms");
	blink_args_1.view = arg_lit0("v", NULL, "View the parameters of the led");
	blink_args_1.end = arg_end(2);
	
	const esp_console_cmd_t cmd_1 = {
		.command = "blink1",
		.help = "Manage the blinking LEDs.",
		.hint = NULL,
		.func = &blinkCommand1,
		.argtable = &blink_args_1
	};
	
	blink_args_2.name = arg_str1(NULL, NULL, "<string1>", "\"led0\" or 0, \"led1\" or 1");
	blink_args_2.param = arg_str1(NULL, NULL, "<string2>", "\"enable\", \"period\", and \"view\"");
	blink_args_2.value = arg_str0(NULL, NULL, "<string3>", "\"on\" or \"off\", integer");
	blink_args_2.end = arg_end(2);
	
	const esp_console_cmd_t cmd_2 = {
		.command = "blink2",
		.help = "A alternate approach to manage the blinking LEDs.",
		.hint = NULL,
		.func = &blinkCommand2,
		.argtable = &blink_args_2
	};
	
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_1));
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_2));
}

// Private Functions

static void blinkTask0(void *arg){
	ESP_ERROR_CHECK(gpio_reset_pin(resources[0].pin));
    ESP_ERROR_CHECK(gpio_set_direction(resources[0].pin, GPIO_MODE_OUTPUT));
	
	static bool blinkState = 0;
    while(1){
		blinkState = !blinkState;
		if(resources[0].enable == true) ESP_ERROR_CHECK(gpio_set_level(resources[0].pin, blinkState));
        vTaskDelay((resources[0].period / 2) / portTICK_PERIOD_MS);
    }
}

static void blinkTask1(void *arg){
	ESP_ERROR_CHECK(gpio_reset_pin(resources[1].pin));
    ESP_ERROR_CHECK(gpio_set_direction(resources[1].pin, GPIO_MODE_OUTPUT));
	
	static bool blinkState = 0;
    while(1){
		blinkState = !blinkState;
		if(resources[1].enable == true) ESP_ERROR_CHECK(gpio_set_level(resources[1].pin, blinkState));
        vTaskDelay((resources[1].period / 2) / portTICK_PERIOD_MS);
    }
}

static int blinkCommand1(int argc, char **argv){
	static const char* TAG = "blink";
	
	int nerrors = arg_parse(argc, argv, (void  **) &blink_args_1);
	if (nerrors != 0) {
        arg_print_errors(stderr, blink_args_1.end, argv[0]);
        return 1;
    }
	int led = blink_args_1.led->ival[0];
	if(led < 0 || led > 1){
		ESP_LOGE(TAG, "Invalid LED selection. Must be 0 or 1.");
		return 1;
	}
	
	if(blink_args_1.enable->count != 0){
		resources[led].enable = blink_args_1.enable->ival[0];
	}
	if(blink_args_1.period->count != 0){
		int period = blink_args_1.period->ival[0];
		if(period < MIN_BLINK_PERIOD) period = MIN_BLINK_PERIOD;
		if(period > MAX_BLINK_PERIOD) period = MAX_BLINK_PERIOD;
		resources[led].period = period;
	}
	if(blink_args_1.view->count != 0){
		printf("enable: %d\n", resources[led].enable);
		printf("period: %d\n", resources[led].period);
	}
	
	return 0;
}

static int blinkCommand2(int argc, char **argv){
	static const char* TAG = "blink2";
	
	int nerrors = arg_parse(argc, argv, (void  **) &blink_args_2);
	if (nerrors != 0) {
        arg_print_errors(stderr, blink_args_2.end, argv[0]);
        return 1;
    }
	
	const char *name = blink_args_2.name->sval[0];
	const char *param = blink_args_2.param->sval[0];
	const char *value = blink_args_2.value->sval[0];
	
	int led;
	if(strcmp(name, "led0") == 0 || strcmp(name, "0") == 0){
		led = 0;
	}else if(strcmp(name, "led1") == 0 || strcmp(name, "1") == 0){
		led = 1;
	}else{
		ESP_LOGE(TAG, "Invalid LED selection. Must be \"led0\", \"led1\", 0 or 1.");
		return 1;
	}
	
	if(strcmp(param, "enable") == 0){
		if(strcmp(value, "on") == 0){
			resources[led].enable = true;
		}else if(strcmp(value, "off") == 0){
			resources[led].enable = false;
		}else{
			ESP_LOGE(TAG, "Invalid enable value. Must be \"on\" or \"off\"");
			return 1;
		}
	}else if(strcmp(param, "period") == 0){
		char *endPointer;
		int period = strtol(value, &endPointer, 10);
		if(*endPointer != '\0'){
			ESP_LOGE(TAG, "Invalid period value. Must be an integer.");
			return 1;
		}
		if(period < MIN_BLINK_PERIOD) period = MIN_BLINK_PERIOD;
		if(period > MAX_BLINK_PERIOD) period = MAX_BLINK_PERIOD;
		resources[led].period = period;
	}else if(strcmp(param, "view") == 0){
		printf("enable: %s\n", resources[led].enable ? "on":"off");
		printf("period: %d\n", resources[led].period);
	}else{
		ESP_LOGE(TAG, "Invalid parameter selection. Must be enable or period");
		return 1;
	}
	
	return 0;
}