/********************************************************************************
 * File Name          : spi.h
 * Author             : Jack Shaver
 * Date               : 4/1/2025
 * Description        : SPI Bus Driver
 ********************************************************************************/
 
#ifndef spi_h
#define spi_h

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/gpio.h"

extern void spiInit(gpio_num_t misoPin, gpio_num_t mosiPin, gpio_num_t clkPin, gpio_num_t csPin);
extern void spiDeinit();

// Single channel read
extern uint16_t spiAdcRead(int channel);
extern float spiAdcReadFloat(int channel);

extern void spiAdcSetSequence(uint8_t channelMask);
extern void spiAdcGetSequence(uint16_t *data);
//extern void spiAdcGetSequenceFloat(float *data);

// Console Interface
extern void spiRegisterCommands();

#ifdef __cplusplus
}
#endif

#endif