/********************************************************************************
 * File Name          : i2c.c
 * Author             : Jack Shaver
 * Date               : 3/31/2025
 * Description        : I2C Bus Driver
 ********************************************************************************/
 
#include "i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h" // command creation
#include "driver/i2c_master.h"

// I2C Device Addresses, add additional addresses here
#define INPUT_EXPANDER_DEVICE_ADDR                  0x20
#define OUTPUT_EXPANDER_DEVICE_ADDR                 0x21

// GPIO Expander Device Numbers
#define EXPANDER_INPUT_NUMBER                       0
#define EXPANDER_OUTPUT_NUMBER                      1

// GPIO Expander Register Addresses
#define EXPANDER_INPUT_REG_ADDR                     0x00
#define EXPANDER_OUTPUT_REG_ADDR                    0x01
#define EXPANDER_POLARITY_REG_ADDR                  0x02
#define EXPANDER_CONFIGURATION_REG_ADDR             0x03

// Struct to hold all the handles, add additional handles here
typedef struct {
    i2c_master_bus_handle_t masterHandle;
    i2c_master_dev_handle_t gpioExpanderHandles[2];
} i2cHandleStruct ;

static i2cHandleStruct handles;
static uint8_t i2cGpioTracker[2] = {0}; // Local Tracking of GPIO
SemaphoreHandle_t i2cSemaphore = NULL; 

// Local Initialization functions
static void masterInit(i2c_master_bus_handle_t *busHandle, gpio_num_t sclPin, gpio_num_t sdaPin);
static void addDevice(i2c_master_bus_handle_t busHandle, i2c_master_dev_handle_t *deviceHandle, uint8_t deviceAddress);
static void removeDevice(i2c_master_dev_handle_t deviceHandle);

// General purpose i2c communication functions 
static void writeToAddress(i2c_master_dev_handle_t deviceHandle, const uint8_t address, const uint8_t data);
static void readFromAddress(i2c_master_dev_handle_t deviceHandle, const uint8_t address, uint8_t *data);
static void writeTwoToAddress(i2c_master_dev_handle_t deviceHandle, const uint8_t address, const uint8_t *data); // not used
static void readTwoFromAddress(i2c_master_dev_handle_t deviceHandle, const uint8_t address, uint8_t *data); // not used

// GPIO expander specific functions
static void writeToExpander(const uint8_t device, const uint8_t address, const uint8_t data);
static void readFromExpander(const uint8_t device, const uint8_t address, uint8_t *data);

static void setExpanderOutputRegister(const uint8_t device, const uint8_t data); // only works on output expander, quiet fail
static uint8_t getExpanderInputRegister(const uint8_t device); // works on both expanders
static void setExpanderOutputBit(const uint8_t device,  const uint8_t bitMask, bool value); // output expander
static bool getExpanderInputBit(const uint8_t device, const uint8_t bitMask); // both

static void inputExpanderInit();
static void outputExpanderInit();

void i2cInit(const gpio_num_t sclPin, const gpio_num_t sdaPin){
	i2cSemaphore = xSemaphoreCreateBinary();    
	xSemaphoreGive(i2cSemaphore); // This was not clear as per the docs
	
    masterInit(&handles.masterHandle, sclPin, sdaPin);

    // Add the I2C Devices to the bus and store the handle
    addDevice(handles.masterHandle, &handles.gpioExpanderHandles[EXPANDER_INPUT_NUMBER], INPUT_EXPANDER_DEVICE_ADDR);
    addDevice(handles.masterHandle, &handles.gpioExpanderHandles[EXPANDER_OUTPUT_NUMBER], OUTPUT_EXPANDER_DEVICE_ADDR);

    // Initialize the I2C Devices
    inputExpanderInit();
    outputExpanderInit();
}

void i2cDeinit(){
	removeDevice(handles.gpioExpanderHandles[EXPANDER_INPUT_NUMBER]);
    removeDevice(handles.gpioExpanderHandles[EXPANDER_OUTPUT_NUMBER]);
}

static struct {
	struct arg_int *device;
    struct arg_int *write;
	struct arg_int *bit;
	struct arg_lit *read;
    struct arg_end *end;
} i2c_args;

static int i2cGpioDevCommand(int argc, char **argv){
	static const char* TAG = "dev-i2c-gpio";
	
	int nerrors = arg_parse(argc, argv, (void  **) &i2c_args);
	if (nerrors != 0) {
        arg_print_errors(stderr, i2c_args.end, argv[0]);
        return 1;
    }
	int device = i2c_args.device->ival[0];
	if(device < 0 || device > 1){
		ESP_LOGE(TAG, "Invalid device selection. Must be 0 or 1.");
		return 1;
	}
	
	if(i2c_args.write->count != 0){ // Write to the device
		int value = i2c_args.write->ival[0];
		if(value < 0 || value > 255){
			ESP_LOGE(TAG, "Invalid value input. Must be 8 bit unsigned.");
			return 1;
		}
		
		if(i2c_args.bit->count != 0){ // Bit select argument
			int bit = i2c_args.bit->ival[0];
			if(bit < 0 || bit > 7){
				ESP_LOGE(TAG, "Invalid bit selection. Must be 0 to 7.");
				return 1;
			}
			setExpanderOutputBit(device, (1 << bit), value); // write a single bit of the output register
		}else{
			setExpanderOutputRegister(device, value); // write the entire output register
		}
	}
	
	if(i2c_args.read->count != 0){ // Query the device
		if(i2c_args.bit->count != 0){ // bit select argument
			int bit = i2c_args.bit->ival[0];
			if(bit < 0 || bit > 7){
				ESP_LOGE(TAG, "Invalid bit selection. Must be 0 to 7.");
				return 1;
			}
			bool temp = getExpanderInputBit(device, (1 << bit)); // get a single bit of the input register
			printf("%d\n", temp);
		}else{
			printf("0x%02x\n", getExpanderInputRegister(device)); // print 2 hex digits, fill 0s
		}
	}
	
	return 0;
}

void i2cRegisterCommands(){
	i2c_args.device = arg_int1(NULL, NULL, "<0|1>", "I2C Device Number (0: inputs, 1: outputs)");
	i2c_args.write = arg_int0("w", NULL, "<uint8>", "Write value to output register");
	i2c_args.bit = arg_int0("b", NULL, "<0-7>", "Address a specifit bit");
	i2c_args.read = arg_lit0("r", NULL, "Read value from input register");
	i2c_args.end = arg_end(2);
	
	const esp_console_cmd_t cmd = {
		.command = "dev-i2c",
		.help = "Manage the i2c gpio expanders.",
		.hint = NULL,
		.func = &i2cGpioDevCommand,
		.argtable = &i2c_args
	};
	
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}


// Reading from i2c gpio 
bool i2cGetGpioSignal(uint16_t i2cMask){
	uint8_t address = i2cMask >> 8;
	uint8_t bitMask = i2cMask & 0xFF;
	return getExpanderInputBit(address, bitMask);
}

// Writing to i2c gpio (only for output expander, no error if improperly used)
void i2cSetGpioSignal(uint16_t i2cMask, bool value){
	uint8_t address = i2cMask >> 8;
	uint8_t bitMask = i2cMask & 0x00FF;
	setExpanderOutputBit(address, bitMask, value);
}

void i2cGetInterruptSources(uint8_t *diff, uint8_t *values){
	uint8_t temp = 0;
    readFromExpander(EXPANDER_INPUT_NUMBER, EXPANDER_INPUT_REG_ADDR, &temp);
	*values = temp;
	*diff = temp ^ i2cGpioTracker[EXPANDER_INPUT_NUMBER]; // generate mask of differences
    i2cGpioTracker[EXPANDER_INPUT_NUMBER] = temp; // save the input register locally
}

// Initializes an I2C Master 
static void masterInit(i2c_master_bus_handle_t *busHandle, gpio_num_t sclPin, gpio_num_t sdaPin){
    i2c_master_bus_config_t masterConfig = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1, // autoselecting
        .scl_io_num = sclPin,
        .sda_io_num = sdaPin,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&masterConfig, busHandle));
}

// Adds an I2C device to the bus
static void addDevice(i2c_master_bus_handle_t busHandle, i2c_master_dev_handle_t *deviceHandle, uint8_t deviceAddress){
    i2c_device_config_t deviceConfig = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = deviceAddress,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(busHandle, &deviceConfig, deviceHandle));
}

// Removes an I2C Device from the bus
static void removeDevice(i2c_master_dev_handle_t deviceHandle){
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(deviceHandle));
}

// Single byte address write and single byte data write
static void writeToAddress(i2c_master_dev_handle_t deviceHandle, const uint8_t address, const uint8_t data){
	if(xSemaphoreTake(i2cSemaphore, 0xffff) == pdTRUE ){
		const uint8_t transmitBuffer[] = {address, data};
		ESP_ERROR_CHECK(i2c_master_transmit(deviceHandle, transmitBuffer, 2, -1));  
		xSemaphoreGive(i2cSemaphore); 
    }		
}

// Single byte address write and single byte data read
static void readFromAddress(i2c_master_dev_handle_t deviceHandle, const uint8_t address, uint8_t *data){
	if(xSemaphoreTake(i2cSemaphore, 0xffff) == pdTRUE ){
		ESP_ERROR_CHECK(i2c_master_transmit_receive(deviceHandle, &address, 1, data, 1, -1));
		xSemaphoreGive(i2cSemaphore); 
    }		
}

// Single byte address write and two byte data write (ADC 2 byte registers)
// data[1]: bits 15 down to 8
// data[0]: bits 7 down to 0
static void writeTwoToAddress(i2c_master_dev_handle_t deviceHandle, const uint8_t address, const uint8_t *data){
    const uint8_t transmitBuffer[] = {address, data[0], data[1]};
	if(xSemaphoreTake(i2cSemaphore, 0xffff) == pdTRUE ){
		ESP_ERROR_CHECK(i2c_master_transmit(deviceHandle, transmitBuffer, 3, -1));    
		xSemaphoreGive(i2cSemaphore); 
    }		
}

// Single byte address write and two byte data read (ADC 2 byte registers)
// data[1]: bits 15 down to 8
// data[0]: bits 7 down to 0
static void readTwoFromAddress(i2c_master_dev_handle_t deviceHandle, const uint8_t address, uint8_t *data){
	if(xSemaphoreTake(i2cSemaphore, 0xffff) == pdTRUE ){
		ESP_ERROR_CHECK(i2c_master_transmit_receive(deviceHandle, &address, 1, data, 2, -1));
		xSemaphoreGive(i2cSemaphore); 
    }	
}

// Write byte to register address of a GPIO Expander (TCA9534)
static void writeToExpander(const uint8_t device, const uint8_t address, const uint8_t data){
    if(device > 1){
        return; // out of bounds
    }
	
    writeToAddress(handles.gpioExpanderHandles[device], address, data);  
}

// Read byte from register address of a GPIO Expander (TCA9534)
static void readFromExpander(const uint8_t device, const uint8_t address, uint8_t *data){
    if(device > 1){
        return; // out of bounds
    }
    readFromAddress(handles.gpioExpanderHandles[device], address, data);
}

static void setExpanderOutputRegister(const uint8_t device, const uint8_t data){ // only works on output expander, quiet fail
	if(device == EXPANDER_OUTPUT_NUMBER){
		i2cGpioTracker[device] = data; // store the new value written to the output register of the output expander
		uint8_t temp = data ^ 0x8F; // invert the active low signals of the output expander
		writeToExpander(device, EXPANDER_OUTPUT_REG_ADDR, temp);
	}else if(device == EXPANDER_INPUT_NUMBER){
		writeToExpander(device, EXPANDER_OUTPUT_REG_ADDR, data);
	}
}

static uint8_t getExpanderInputRegister(const uint8_t device){ // works on both expanders
	if(device == EXPANDER_INPUT_NUMBER){
		readFromExpander(device, EXPANDER_INPUT_REG_ADDR, &i2cGpioTracker[device]); // read the input register into the tracker
		return i2cGpioTracker[device];
	}else if(device == EXPANDER_OUTPUT_NUMBER){
		uint8_t temp;
		readFromExpander(device, EXPANDER_INPUT_REG_ADDR, &temp);
		temp = temp ^ 0x8F; // invert active low signals
		i2cGpioTracker[device] = temp;
		return temp;
	}
	return 0;
}

// Device must be 0 or 1, bitmask must be 1 bit of an 8 bit number
static void setExpanderOutputBit(const uint8_t device,  const uint8_t bitMask, bool value){
	uint8_t temp = 0;
	if(value == 0){
		temp = i2cGpioTracker[device] & ~bitMask; // Unset the bits of the bitmask into the tracker
	}else{
		temp = i2cGpioTracker[device] | bitMask; // Set the bits of the bitmask into the tracker
	}
	setExpanderOutputRegister(device, temp);
}

// Device must be 0 or 1, bitmask must be 1 bit of an 8 bit number
static bool getExpanderInputBit(const uint8_t device, const uint8_t bitMask){
	return getExpanderInputRegister(device) & bitMask;
}

// Initialize the GPIO Expander used as inputs (TCA9534)
// Stores initial values of the input register to track changes
static void inputExpanderInit(){
    writeToExpander(EXPANDER_INPUT_NUMBER, EXPANDER_POLARITY_REG_ADDR, 0x2F); // Invert the negative logic inputs
    writeToExpander(EXPANDER_INPUT_NUMBER, EXPANDER_CONFIGURATION_REG_ADDR, 0xFF); // Configure all as inputs
    readFromExpander(EXPANDER_INPUT_NUMBER, EXPANDER_INPUT_REG_ADDR, &i2cGpioTracker[EXPANDER_INPUT_NUMBER]); // store initial value of the input register
}

// Initialize the GPIO Expander used as outputs (TCA9534)
static void outputExpanderInit(){
	setExpanderOutputRegister(EXPANDER_OUTPUT_NUMBER, 0x00); // set all signals to low, note, this function inverts active low signals
    //writeToExpander(EXPANDER_OUTPUT_NUMBER, EXPANDER_CONFIGURATION_REG_ADDR, 0x00); // Configure all as outputs
	writeToExpander(EXPANDER_OUTPUT_NUMBER, EXPANDER_CONFIGURATION_REG_ADDR, 0x20); // Configure all as outputs, except sys enable
	// Sys Enable was modded to be permanantly enabled. MCU would reset when it was assertted due to inrush current slumping the power supply
	// Could fix with a board revision, less capacitance on auxially supplies, use the enable signal of the 12V boost converter, more capacitance on main supplies
	readFromExpander(EXPANDER_OUTPUT_NUMBER, EXPANDER_INPUT_REG_ADDR, &i2cGpioTracker[EXPANDER_OUTPUT_NUMBER]);
}