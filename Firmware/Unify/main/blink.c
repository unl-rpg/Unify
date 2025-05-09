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
#define MAX_BLINK_PERIOD 5000

typedef struct{
    gpio_num_t pin;
	bool enable;
	int period;
} blinkStruct;
static blinkStruct blinkConfig;

static struct {
    struct arg_int *enable;
    struct arg_int *period;
	struct arg_lit *query;
    struct arg_end *end;
} blink_args;

static void blinkTask(void *arg);
static int blinkCommand(int argc, char **argv);

void blinkInit(gpio_num_t pin){
	blinkStruct config = {
		.pin = pin,
		.enable = true,
		.period = 1000,
	};
	blinkConfig = config;
	
	ESP_ERROR_CHECK(gpio_reset_pin(blinkConfig.pin));
    ESP_ERROR_CHECK(gpio_set_direction(blinkConfig.pin, GPIO_MODE_OUTPUT));
	xTaskCreate(blinkTask, "blinkTask", 4096, NULL, 1, NULL);
}

bool blinkGetEnable(){
	return blinkConfig.enable;
}

int blinkGetPeriod(){
	return blinkConfig.period;
}

void blinkSetEnable(bool enable){
	blinkConfig.enable = enable;
}

void blinkSetPeriod(int period){
	if(period < MIN_BLINK_PERIOD) period = MIN_BLINK_PERIOD;
	if(period > MAX_BLINK_PERIOD) period = MAX_BLINK_PERIOD;
	blinkConfig.period = period;
}

void blinkRegisterCommands(){
	blink_args.enable = arg_int0("e", NULL, "<0|1>", "Enable or Disable the blinking");
	blink_args.period = arg_int0("p", NULL, "<50-5000>", "Blinking period in ms");
	blink_args.query = arg_lit0("q", NULL, "Query the blink parameters");
	blink_args.end = arg_end(2);
	
	const esp_console_cmd_t cmd = {
		.command = "dev-blink",
		.help = "Manage the blinking LED.",
		.hint = NULL,
		.func = &blinkCommand,
		.argtable = &blink_args
	};
	
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

// Private Functions
static void blinkTask(void *arg){	
	static bool blinkState = 0;
	static bool disableEdge = 1;
    while(1){
		blinkState = !blinkState;
		if(blinkConfig.enable == true){
			ESP_ERROR_CHECK(gpio_set_level(blinkConfig.pin, blinkState));
			disableEdge = 1;
		}else{
			if(disableEdge == 1){ // probably overkill
				disableEdge = 0;
				ESP_ERROR_CHECK(gpio_set_level(blinkConfig.pin, 0));
			}
		}
        vTaskDelay((blinkConfig.period / 2) / portTICK_PERIOD_MS);
    }
}

static int blinkCommand(int argc, char **argv){
	static const char* TAG = "dev-blink";
	
	int nerrors = arg_parse(argc, argv, (void  **) &blink_args);
	if (nerrors != 0) {
        arg_print_errors(stderr, blink_args.end, argv[0]);
        return 1;
    }
	if(blink_args.enable->count != 0){
		blinkConfig.enable = blink_args.enable->ival[0];
	}
	if(blink_args.period->count != 0){
		int period = blink_args.period->ival[0];
		if(period < MIN_BLINK_PERIOD) period = MIN_BLINK_PERIOD;
		if(period > MAX_BLINK_PERIOD) period = MAX_BLINK_PERIOD;
		blinkConfig.period = period;
	}
	if(blink_args.query->count != 0){
		printf("enable: %d\n", blinkConfig.enable);
		printf("period: %d\n", blinkConfig.period);
	}
	
	return 0;
}