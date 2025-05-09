/********************************************************************************
 * File Name          : blink.h
 * Author             : Jack Shaver
 * Date               : 3/24/2025
 * Description        : Blink Header
 ********************************************************************************/
 
#ifndef blink_h
#define blink_h
 
#ifdef __cplusplus
extern "C" {
#endif
 
#include "driver/gpio.h" // gpio_num_t
 
extern void blinkInit(gpio_num_t led0, gpio_num_t led1);
extern int blinkGetEnable(int led, bool* enable);
extern int blinkGetPeriod(int led, int* period);
extern int blinkSetEnable(int led, bool enable);
extern int blinkSetPeriod(int led, int period);

// Used within repl console
extern void blinkRegisterCommands();



#ifdef __cplusplus
}
#endif
 
#endif 
 