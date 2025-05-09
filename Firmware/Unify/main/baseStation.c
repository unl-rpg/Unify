/********************************************************************************
 * File Name          : baseStation.c
 * Author             : Jack Shaver
 * Date               : 4/11/2025
 * Description        : Base Station Source
 ********************************************************************************/
 
#include "baseStation.h"

// Good to have
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

// Used for the console
#include "esp_console.h"
#include "argtable3/argtable3.h"

// Custom Libraries
#include "pins.h" 
#include "blink.h" // blink led
#include "buzzer.h" // buzzer
#include "adc.h" // internal adc
#include "espnow.h"

// Console
static void consoleInit(); 

// System
static void systemRegisterCommands();

// Espnow 
void espnowReceiveCallback(const esp_now_recv_info_t* mac_addr, const unsigned char* data, int len);

// Buzzer Countdown
SemaphoreHandle_t buzzerTaskBlockSemaphore = NULL; // blocks the task until interrupt
static void buzzerTask(void *arg);
static void startCoundown();
static void stopCountdown();

// Base station
static bool checkIsBaseStation(){
	ESP_ERROR_CHECK(gpio_reset_pin(CONFIG_PIN));
    ESP_ERROR_CHECK(gpio_set_direction(CONFIG_PIN, GPIO_MODE_INPUT));
	return gpio_get_level(CONFIG_PIN); // Returns 1 if its a base station
}

void baseStationInit(){
	
	if(checkIsBaseStation() != true){
		printf("Cannot configure hardware as a BaseStation. Detected hardware configuration for TestStand.\n");
		printf("Aborting Initialization.\n");
		return;
	}
	
	blinkInit(BLINK_PIN);
	buzzerInit(BUZZER_PIN);
	adcInit(); // This is the internal ADC, not the spi ADC
	
	espnowInit(espnowTestStandMac);
	espnowRegisterRecieveCallback(espnowReceiveCallback);
	
	// Init the task for managing the fire sequence
	buzzerTaskBlockSemaphore = xSemaphoreCreateBinary();
	xTaskCreate(buzzerTask, "buzzerTask", 4096, NULL, 1, NULL);

	//espnowGetMAC(espnowBaseStationMac);
	//printf("Base Station MAC Address: 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x", 
	//	espnowBaseStationMac[0], espnowBaseStationMac[1], espnowBaseStationMac[2], 
	//	espnowBaseStationMac[3], espnowBaseStationMac[4], espnowBaseStationMac[5]
	//);
	
	consoleInit(); // Do this last
}

// ================================= ESPNOW RECIEVE =====================================
void espnowReceiveCallback(const esp_now_recv_info_t* mac_addr, const unsigned char* data, int len){
	uint16_t command = data[0];
	printf("Espnow Recieved Command: 0x%02x\n", command);
	
	switch(command){
	case espnowAcknowledgeCommand:
		printf("Recieved acknowledge from Test Stand\n\n");
		break;
	case espnowPingCommand:
		espnowSendCommand(espnowAcknowledgeCommand);
		break;
	case espnowUnrecognizedCommand:
		printf("Test Stand did not recognize last Espnow Command\n\n");
		break;
	case espnowConfirmCountdown:
		printf("Test Stand started the countdown.\n\n");
		startCoundown();
		break;
	case espnowAbortConfirmationCommand:
		printf("Test Stand confirmed the Abort command.\n\n");
		stopCountdown();
		break;
	case espnowBadKeyStateCommand:
		printf("Test Stand safety key is locked, did not fire.\n\n");
		break;
	case espnowNoSdCardCommand:
		printf("Test Stand SD Card is not inserted, did not fire.\n\n");
		break;
	case espnowBadIgniterCommand:
		printf("Test Stand igniter is not connected or is used, did not fire.\n\n");
		break;
	case espnowGoodFireCommand:
		printf("Test Stand confirmed a good ignition of ematch.\n\n");
		break;
	case espnowBadFireCommand:
		printf("Test Stand detected a failed ignition of ematch, aproach with caution!\n\n");
		break;
	default:
		printf("Unrecognized Espnow Command\n\n");
		espnowSendCommand(espnowUnrecognizedCommand);
		break;
	}
}

// ================================= SYSTEM CONSOLE INTERFACE ===============================================
static struct {
	struct arg_lit *querySystemType;
	struct arg_lit *ping;
	struct arg_lit *temperature;
	struct arg_lit *battery;
	struct arg_lit *powerOff; 
	struct arg_lit *fire;
	struct arg_lit *abort;
    struct arg_end *end;
} system_cmd_args;

// System is asymmetrical between the base station and test stand
static int systemCommands(int argc, char **argv){
	static const char* TAG = "system-commands";
	
	int nerrors = arg_parse(argc, argv, (void  **) &system_cmd_args);
	if (nerrors != 0) {
        arg_print_errors(stderr, system_cmd_args.end, argv[0]);
        return 1;
    }
	
	if(system_cmd_args.querySystemType->count != 0){
		printf("Base Station\n\n");
	}
	
	if(system_cmd_args.ping->count != 0){ // ping the other end of the espnow connection
		printf("Pinging the Test Stand...\n\n");
		espnowSendCommand(espnowPingCommand);
	}
	
	if(system_cmd_args.temperature->count != 0){
		printf("PCB Temperature: %2.6f C\n\n", adcRead(pcbTemperature));
	}
	
	if(system_cmd_args.battery->count != 0){
		printf("Battery Voltage: %2.6f V\n", adcRead(batteryVoltage));
		printf("Note: Battery voltage is inacurate due to supply from USB.\n\n");
	}
	
	if(system_cmd_args.powerOff->count != 0){ // power off the device
		printf("Base Station cannot power off due to I2C short, unplug USB instead.\n\n");
	}
	
	if(system_cmd_args.fire->count != 0){ // send the fire command to start countdown and measuring
		printf("Starting Test Stand Countdown...\n\n");
		espnowSendCommand(espnowFireCommand);
	}
	
	if(system_cmd_args.abort->count != 0){ // send the abort command to stop countdown and measuring
		printf("Aborting Test Stand Countdown...\n\n");
		espnowSendCommand(espnowAbortCommand);
	}
	
	return 0;
}

void systemRegisterCommands(){
	system_cmd_args.querySystemType = arg_lit0("q", "query", "Returns the system type");
	system_cmd_args.ping = arg_lit0("p", "ping", "Pings the test stand");
	system_cmd_args.temperature = arg_lit0("t", "temperature", "Checks the PCB temperature");
	system_cmd_args.battery = arg_lit0("b", "battery", "Checks the battery voltage");
	system_cmd_args.fire = arg_lit0("f", "fire", "Begins the coundown, begins logging, and ignites motor on test stand");
	system_cmd_args.abort = arg_lit0("a", "abort", "Aborts the coundown, or stops the logging on the test stand");
	system_cmd_args.powerOff = arg_lit0("o", "off", "Powers off the system");
	system_cmd_args.end = arg_end(2);
	
	const esp_console_cmd_t cmd = {
		.command = "system",
		.help = "Control the local system",
		.hint = NULL,
		.func = &systemCommands,
		.argtable = &system_cmd_args
	};
	
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

// ================================= CONSOLE ==============================================
void consoleInit(){
	//initialize_nvs();
	
	esp_console_register_help_command();
	systemRegisterCommands();
	//blinkRegisterCommands();
	buzzerRegisterCommands();
	//adcRegisterCommands();
	
	esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
	repl_config.prompt = "Unify BaseStation>";
    repl_config.max_cmdline_length = 1024;
	
	// Make sure to set CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y in the sdkconfig file. 
	// esp_console.h uses this config to enable the USB Serial JTAG console functions
	esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
	
	ESP_ERROR_CHECK(esp_console_start_repl(repl));
}

// ================================= BUZZER COUNTDOWN ==============================================

static bool countdownStop = false;
static int countdown = 0;
void startCoundown(){
	countdownStop = false;
	countdown = 5; 
	xSemaphoreGive(buzzerTaskBlockSemaphore);
}

static void stopCountdown(){
	countdownStop = true;
}

void buzzerTask(void *arg){
	 while(1){
        if(xSemaphoreTake(buzzerTaskBlockSemaphore, 0xffff) == pdTRUE){ 
			buzzerSetEnable(true);
			do{
				if(countdownStop == true){
					break;
				}
				countdown--;
				vTaskDelay(1000/ portTICK_PERIOD_MS);
			}while(countdown > 0);
			buzzerSetEnable(false);
			countdownStop = false;
		}
	 }
}