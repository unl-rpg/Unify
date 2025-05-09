/********************************************************************************
 * File Name          : console_test.c
 * Author             : Jack Shaver
 * Date               : 3/23/2025
 * Description        : Top Level of the Console Test Application
 ********************************************************************************/

// Good to have on all projects
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
//#include "driver/gpio.h" 

// Stores the commands
#include "nvs.h"
#include "nvs_flash.h"

// Used for creating commands
//#include "argtable3/argtable3.h"

// Used for console creation, make sure to create the sdkconfig CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
#include "esp_console.h"

// My files
#include "blink.h"
#include "adc.h"


// Specific to the breadboard I am testing on
#define BUTTON_PIN GPIO_NUM_0
#define BLINK_LED_0_PIN GPIO_NUM_1
#define BLINK_LED_1_PIN GPIO_NUM_2
#define POTENTIOMETER_PIN GPIO_NUM_3



// Nonvolatile flash used for storing commands
static void initialize_nvs(void){
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void consoleInit(){
	initialize_nvs();
	
	esp_console_register_help_command();
	blinkRegisterCommands();
	
	esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
	repl_config.prompt = "Unify>";
    repl_config.max_cmdline_length = 1024;
	
	// Make sure to set CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y in the sdkconfig file. 
	// esp_console.h uses this config to enable the USB Serial JTAG console functions
	esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
	
	ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
	

void app_main(void){
	blinkInit(BLINK_LED_0_PIN, BLINK_LED_1_PIN);
	consoleInit();
}
