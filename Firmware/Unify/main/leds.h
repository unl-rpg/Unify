/********************************************************************************
 * File Name          : leds.h
 * Author             : Jack Shaver
 * Date               : 4/7/2025
 * Description        : Runs the Power, Charge and Armed LEDs
 ********************************************************************************/
 
#ifndef leds_h
#define leds_h
 
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	ledPower = 0, ledCharge = 1, ledStatus = 2
} ledsType;

typedef enum{
    ledOff, ledOn, ledFlashing
} ledsStateType;
 
extern void ledsInit();

extern void ledsSetState(ledsType led, ledsStateType state);
extern void ledsSetPeriod(ledsType led, int period);

extern ledsStateType ledsGetState(ledsType led);
extern int ledsGetPeriod(ledsType led);

extern void ledsReportError(int error); // two digit number
extern void ledsClearError();

// Used within repl console
extern void ledsRegisterCommands();

#ifdef __cplusplus
}
#endif
 
#endif 
 