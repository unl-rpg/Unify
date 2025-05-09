/********************************************************************************
 * File Name          : buzzer.h
 * Author             : Jack Shaver
 * Date               : 4/1/2025
 * Description        : Buzzer Header
 ********************************************************************************/
 
#ifndef buzzer_h
#define buzzer_h
 
#ifdef __cplusplus
extern "C" {
#endif
 
#include "driver/gpio.h" // gpio_num_t
 
extern void buzzerInit(gpio_num_t pin);
extern bool buzzerGetEnable();
extern int buzzerGetPeriod();
extern int buzzerGetDutyCycle();
extern void buzzerSetEnable(bool enable);
extern void buzzerSetPeriod(int period);
extern void buzzerSetDutyCycle(int dutyCycle);

extern void buzzerSetNumberOfBeeps(int num);

// Used within repl console
extern void buzzerRegisterCommands();



#ifdef __cplusplus
}
#endif
 
#endif 
 