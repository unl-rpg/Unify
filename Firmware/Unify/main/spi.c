/********************************************************************************
 * File Name          : spi.c
 * Author             : Jack Shaver
 * Date               : 4/1/2025
 * Description        : SPI Bus Driver
 ********************************************************************************/
 
#include "spi.h"

#include <string.h> //memset

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h" // command creation

#include "hal/spi_types.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

// Used to address multiple spi slaves on the SPI2_HOST
#define MAX_DEVICES 1
#define ADC_DEVICE_NUMBER 0

// Used to track the mode of the adc
static bool sequencerMode = 0; // adc starts in manual mode

// Store the device handles, overbuilt for this application
static spi_device_handle_t deviceHandles[MAX_DEVICES] = {0}; 

// Semaphore to create thread safe polling during transactions, could look into using the spi queue
SemaphoreHandle_t spiSemaphore = NULL; 

// SPI Bus Functions
static void initBus(gpio_num_t misoPin, gpio_num_t mosiPin, gpio_num_t clkPin);
static void deinitBus();

// SPI Device Functions, use unique deviceNumbers for each device
static void addDevice(gpio_num_t csPin, int speed, uint8_t mode, int deviceNumber);
static void removeDevice(int deviceNumber);

// Thread safe generic i2c interface
void genericTransmit(int deviceNumber, uint8_t *data, uint8_t len);
void genericRecieve(int deviceNumber, uint8_t *data, uint8_t len);

// Basic adc interface
static void adcInit();
static void adcWriteRegister(uint8_t address, uint8_t data);
static uint8_t adcReadRegister(uint8_t address);
static void adcWriteBit(uint8_t address, uint8_t bit, bool value);
static bool adcReadBit(uint8_t address, uint8_t bit);
static uint16_t adcMeasureChannel(int channel); // 12 bit data, left padded 0s


extern void spiInit(gpio_num_t misoPin, gpio_num_t mosiPin, gpio_num_t clkPin, gpio_num_t csPin){
	spiSemaphore = xSemaphoreCreateBinary();    
	xSemaphoreGive(spiSemaphore);
	
	initBus(misoPin, mosiPin, clkPin);
	addDevice(csPin, SPI_MASTER_FREQ_20M, 0, ADC_DEVICE_NUMBER);
	
	vTaskDelay(100 / portTICK_PERIOD_MS); // testing
	adcInit();
}

extern void spiDeinit(){
	removeDevice(ADC_DEVICE_NUMBER);
	deinitBus();
}

uint16_t spiAdcRead(int channel){
	return adcMeasureChannel(channel);
}

extern float spiAdcReadFloat(int channel){
	//return (adcMeasureChannel(channel) / 4096.0) * 5.0;
	return (adcMeasureChannel(channel) / 65535.0) * 5.0;
}

// Stuff for the console
static struct {
	struct arg_int *address; // read/write from register at address
    struct arg_int *write; // value to write
	struct arg_lit *read; // read from address
	struct arg_int *bit; // sepcify bit position fro read/write
    struct arg_end *end;
} spi_reg_args;

static int spiRegDevCommand(int argc, char **argv){
	static const char* TAG = "dev-spi-reg";
	
	int nerrors = arg_parse(argc, argv, (void  **) &spi_reg_args);
	if (nerrors != 0) {
        arg_print_errors(stderr, spi_reg_args.end, argv[0]);
        return 1;
    }
	int address = spi_reg_args.address->ival[0];
	
	if(spi_reg_args.write->count != 0){ // Write to the device
		int value = spi_reg_args.write->ival[0];
		if(value < 0 || value > 255){
			ESP_LOGE(TAG, "Invalid write input. Must be 8 bit unsigned.");
			return 1;
		}
		if(spi_reg_args.bit->count != 0){
			int bit = spi_reg_args.bit->ival[0];
			if(bit < 0 || bit > 7){
				ESP_LOGE(TAG, "Invalid bit input. Must be between 0 and 7.");
				return 1;
			}
			adcWriteBit(address, bit, value);
		}else{
			adcWriteRegister(address, value);
		}
	}
	
	if(spi_reg_args.read->count != 0){ // Query the device
		if(spi_reg_args.bit->count != 0){
			int bit = spi_reg_args.bit->ival[0];
			if(bit < 0 || bit > 7){
				ESP_LOGE(TAG, "Invalid bit input. Must be between 0 and 7.");
				return 1;
			}
			printf("%x\n", adcReadBit(address, bit));
		}else{
			printf("0x%02x\n", adcReadRegister(address)); // print 2 hex digits, fill 0s
		}
	}
	
	return 0;
}

static struct {
	struct arg_int *measure; // Read a single channel
    struct arg_int *sequence; // Run Sequencer over masked channels
	struct arg_lit *convert; // Convert to floats
    struct arg_end *end;
} spi_adc_args;

static int spiAdcDevCommand(int argc, char **argv){
	static const char* TAG = "dev-spi-adc";
	
	int nerrors = arg_parse(argc, argv, (void  **) &spi_adc_args);
	if (nerrors != 0) {
        arg_print_errors(stderr, spi_adc_args.end, argv[0]);
        return 1;
    }
	
	if(spi_adc_args.measure->count != 0){ 
		int channel = spi_adc_args.measure->ival[0];
		if(channel < 0 || channel > 7){
			ESP_LOGE(TAG, "Invalid channel input. Must be between 0 and 7.");
			return 1;
		}
		if(spi_adc_args.convert->count != 0){
			printf("%2.6f\n", spiAdcReadFloat(channel));
		}else{
			printf("0x%04x\n", spiAdcRead(channel));
		}
	}
	
	if(spi_adc_args.sequence->count != 0){
		printf("Not yet implemented.\n");
	}
	
	
	return 0;
}

void spiRegisterCommands(){
	spi_reg_args.address = arg_int1(NULL, NULL, "<uint8>", "ADC Register to address, avoid addressing invalid registers!");
	spi_reg_args.write = arg_int0("w", NULL, "<uint8>", "Write a value to the address");
	spi_reg_args.read = arg_lit0("r", NULL, "Read a value from the address");
	spi_reg_args.bit = arg_int0("b", NULL, "<0-7>", "Address a bit position of the register");
	spi_reg_args.end = arg_end(2);
	
	const esp_console_cmd_t cmd_reg = {
		.command = "dev-spi-reg",
		.help = "Manage the spi ADC registers. ",
		.hint = NULL,
		.func = &spiRegDevCommand,
		.argtable = &spi_reg_args
	};
	
	spi_adc_args.measure = arg_int0("m", NULL, "<0-7>", "Measure a single ADC analog input");
	spi_adc_args.sequence = arg_int0("s", NULL, "<uint8>", "Run the ADC sequencer over the channels in the mask");
	spi_adc_args.convert = arg_lit0("f", NULL, "Display results as floating point");
	spi_adc_args.end = arg_end(2);
	
	const esp_console_cmd_t cmd_adc = {
		.command = "dev-spi-adc",
		.help = "Manage the spi ADC analog inputs.",
		.hint = NULL,
		.func = &spiAdcDevCommand,
		.argtable = &spi_adc_args
	};
	
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_reg));
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_adc));
}

// Private functions
void initBus(gpio_num_t misoPin, gpio_num_t mosiPin, gpio_num_t clkPin){
	spi_bus_config_t buscfg = {
		.miso_io_num = misoPin,
		.mosi_io_num = mosiPin,
		.sclk_io_num = clkPin,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 32
	};
	
	ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
}

void deinitBus(){ // Only works after all devices have been removed
	ESP_ERROR_CHECK(spi_bus_free(SPI2_HOST));
}

void addDevice(gpio_num_t csPin, int speed, uint8_t mode, int deviceNumber){
	spi_device_interface_config_t devcfg = {
		.mode = mode, 
		.clock_speed_hz = speed, 
		.spics_io_num = csPin,
		.queue_size = 1, // only 1 in the queue, use semaphores when accessing the driver
		//.pre_cb =  // no pre transfer call back, could be used to set gpio other than cs
		//.post_cb =
		.cs_ena_pretrans = 4
	};
	
	ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &deviceHandles[deviceNumber]));
}

void removeDevice(int deviceNumber){
	ESP_ERROR_CHECK(spi_bus_remove_device(deviceHandles[deviceNumber]));
}

void genericTransmit(int deviceNumber, uint8_t *data, uint8_t len){
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = 8 * len;
	t.tx_buffer = data;
	
	if(xSemaphoreTake(spiSemaphore, 0xffff) == pdTRUE ){
		ESP_ERROR_CHECK(spi_device_polling_transmit(deviceHandles[deviceNumber], &t));
		xSemaphoreGive(spiSemaphore); 
    }	
}

void genericRecieve(int deviceNumber, uint8_t *data, uint8_t len){
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	t.length = 8 * len;
	t.rx_buffer = data;
	
	if(xSemaphoreTake(spiSemaphore, 0xffff) == pdTRUE ){
		ESP_ERROR_CHECK(spi_device_polling_transmit(deviceHandles[deviceNumber], &t));
		xSemaphoreGive(spiSemaphore); 
    }	
}


//================================== ADC Interface ===========================================

void adcWriteRegister(uint8_t address, uint8_t data){
	uint8_t txBuffer[3] = {0x08, address, data}; // write command, register address, data
	genericTransmit(ADC_DEVICE_NUMBER, txBuffer, 3);
}

uint8_t adcReadRegister(uint8_t address){
	uint8_t txBuffer[3] = {0x10, address, 0x00}; // read command, register address, dummy
	genericTransmit(ADC_DEVICE_NUMBER, txBuffer, 3);

	uint8_t rxBuffer[3] = {0}; // read 3 bytes for timing
	genericRecieve(ADC_DEVICE_NUMBER, rxBuffer, 3);
	return rxBuffer[0];
}

void adcWriteBit(uint8_t address, uint8_t bit, bool value){
	uint8_t temp = adcReadRegister(address);
	if(value == 0){
		temp = temp & ~(1 << bit);
	}else{
		temp = temp | (1 << bit);
	}
	adcWriteRegister(address, temp);
}

bool adcReadBit(uint8_t address, uint8_t bit){
	uint8_t temp = adcReadRegister(address);
	return temp & (1 << bit);
}

void adcInit(){
	if(adcReadBit(0x00, 2)){ // error on startup
		printf("SPI ADC error on startup, automatic reset triggered.");
		adcWriteBit(0x01, 0, 1); // reset the device
		adcWriteBit(0x00, 0, 1); // clear the brown out flag, can check later to see if device is browned out
	}else if(adcReadBit(0,0)){
		printf("SPI ADC brownout detected on initialization.");
		adcWriteBit(0x00, 0, 1); // clear the brown out flag, can check later to see if device is browned out
	}
	
	
	sequencerMode = 0; // starts in manual mode
	adcWriteRegister(0x01, 0x06); // Force all channels to be analog inputs, calibrate ADC offset
	//adcWriteRegister(0x02, 0x10); // Append 4 bit channel ID to the ADC data
	adcWriteRegister(0x02, 0x00);
	//adcWriteRegister(0x02, 0x90); // This makes all adc readings come back as a5a for testing
	adcWriteRegister(0x04, 0x00); // Conv mode = Manual mode
	adcWriteRegister(0x05, 0x00); // pin config as adc inputs, should be irrelevand due to forced overide
	adcWriteRegister(0x10, 0x00); // seq mode = manual mode
	
	//adcWriteRegister(0x03, 0x02); // 4 sample averaging
	adcWriteRegister(0x03, 0x03); // 8 sample averaging
}

// 12 bit data, left padded 0s, only works if not in sequencer mode
// THIS IS RELATIVELY SLOW
static int currentChannel = 0;
uint16_t adcMeasureChannel(int channel){
	
	//if(sequencerMode == 1){ // must put the adc back into manual mode first
	//	adcWriteRegister(0x04, 0x00); // Conv mode = Manual mode
	//	adcWriteRegister(0x10, 0x00); // seq mode = manual mode
	//}
	
	if(channel < 0 || channel > 7){
		printf("Invalid ADC channel to read during single conversion");
		return 0;
	}
	
	if(currentChannel != channel){
		currentChannel = channel;
		uint8_t txBuffer[3] = {0x08, 0x11, channel}; // write command, channel sel register, channel number
		genericTransmit(ADC_DEVICE_NUMBER, txBuffer, 3);
	}

	//vTaskDelay(10/ portTICK_PERIOD_MS); // testing
	uint8_t rxBuffer[3] = {0};
	genericRecieve(ADC_DEVICE_NUMBER, rxBuffer, 3); // Throw away
	genericRecieve(ADC_DEVICE_NUMBER, rxBuffer, 3); // This is valid
	
	// 12bits of data + 4 bits of channel id
	// move the 8MSB data bits into 11 downto 4, move the 4LSB into 3 downto 0, drop the channel id
	//return 0 | (rxBuffer[0] << 4) | (rxBuffer[1] >> 4);
	return 0| (rxBuffer[0] << 8) | rxBuffer[1];
}

void spiAdcSetSequence(uint8_t channelMask){
	if(sequencerMode == 0){ // put into sequencer mode
		adcWriteRegister(0x04, 0x20); // Conv mode = Auto 
		adcWriteRegister(0x10, 0x01); // seq mode = sequencer
	}
	
	adcWriteRegister(0x12, channelMask); // writes the selected channels into the auto seq ch sel register
	adcWriteRegister(0x10, 0x11); // seq mode = sequencer, set start = true
}

void spiAdcGetSequence(uint16_t *data){
	if(sequencerMode == 0){
		// DO NOT LET THIS HAPPEN, i have not thought of a good way to handle this
	}
	
}