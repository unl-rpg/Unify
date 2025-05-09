/********************************************************************************
 * File Name          : Unify.c
 * Author             : Jack Shaver
 * Date               : 3/23/2025
 * Description        : Top Level of the Unify Testing System
 ********************************************************************************/
 
/*
NOTES:
	When Sys_En (i2c gpio expander 1, bit 5) was set, the inrush current would reset the board
		- Changed that bit to be configured as an input
			- Can still read and "write" without issues
			- This bit does not respond to write operations
		- Added mod wire to assert Sys_En on power up
			- Mod wire connects the Sys_En to the 3.3V supply
		- Can no longer disable the auxillary power supplies
		- Measured current increased from 30mA to 40mA (No sensors connected, sd card not configured)
		- Make sure to attact all necessary sensors before powering on
			- DO NOT HOT PLUG SENSORS
		
	With a 1 ohm resistor in place of the igniter, and a 3 ohm resistor network placed into the shunt through holes
		- Igniter current was measured to be 880 mA
		- The resistors got hot after about 10 seconds
		- Make sure to test that this current will ignite the ematch
		- Make sure that the ematch continuity is broken after ignition
			- If not, measure the minimum all fire time it takes to ignite, and assert the igniter signal for that long
		
	Vbus measured operating range is between 2.0V and 5.0V
		- May be drained down to 1.2V, cannot startup from this voltage
		- Do not drain the battery below 2.5V 
		- Current draw increases up to 60mA from 30mA at lower supply voltages
		
	Schematic error where SPI CS and SPI CLK are swapped into the ADC
		- Changed the pin definitions to reflect the swapping
		- The Spi Host should still be routed through the IO MUX, no harm done
			- If not, we only use 20MHz, and Espressif says GPIO matrix is fast enough
			
	The base station has a solder bridge between the i2c sda and scl pins somewhere
		- This prevents the i2c gpio expanders from working, or even being configured
			- This will hang up the program so they have been removed from the code entirely
		- Due to this, the front panel does not work, nor does powering down the device
		- Sensing the USB insertion and power does not work either
			- Must power the board from the USB port to allow the device to be powered down	
				- Remove the usb plug to power down
			- Must use an appropriate USB type C cable which advertises 3 amps to power the board
		- Someone with more time should try to find and fix the solder bridge, or make a new base station
			- I think it is under the ESP32S3, so could try to desolde it and put it back
				- I tried to pop it using the soldering iron and alot of heat, didnt work
*/

/*
DESIGN:
	Main calls the init function for either a base station or a test stand
	Each file appropriately configures the board 
	They share the same drivers but implement them differently
*/

#include "baseStation.h"
#include "testStand.h"

#include "nvs.h"
#include "nvs_flash.h"

// Set isBaseStation to 1 to program a baseStation, Set to 0 to program a testStand
// The init function will confirm against the on board config jumper, will abort if theres a conflict
#define isBaseStation 0

uint8_t espnowBaseStationMac[6] = {0xB4, 0x3A, 0x45, 0xC0, 0x66, 0x38}; 
uint8_t espnowTestStandMac[6] = {0xB4, 0x3A, 0x45, 0xC0, 0x66, 0x34}; 

static void initialize_nvs(void){
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main(void){
	
	initialize_nvs();
	
	#if isBaseStation
		baseStationInit();	
	#else	
		testStandInit();
	#endif
}
