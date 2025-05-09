/********************************************************************************
 * File Name          : adc.h
 * Author             : Jack Shaver
 * Date               : 3/24/2025
 * Description        : ADC Header
 ********************************************************************************/
 
#ifndef adc_h
#define adc_h
 
#ifdef __cplusplus
extern "C" {
#endif
 
#include "driver/gpio.h" // gpio_num_t

extern void adcInit(gpio_num_t pin); // Should make this dynamic in size
extern void adcRead(uint16_t *voltage);
extern void adcReadFloat(float *voltage);

// Used within repl console
extern void adcRegisterCommands();
 
#ifdef __cplusplus
}
#endif

#endif 