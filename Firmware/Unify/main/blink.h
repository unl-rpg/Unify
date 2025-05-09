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
 
extern void blinkInit(gpio_num_t pin);
extern bool blinkGetEnable();
extern int blinkGetPeriod();
extern void blinkSetEnable(bool enable);
extern void blinkSetPeriod(int period);

// Used within repl console
extern void blinkRegisterCommands();



#ifdef __cplusplus
}
#endif
 
#endif 
 