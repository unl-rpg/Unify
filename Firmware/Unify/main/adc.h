/********************************************************************************
 * File Name          : adc.h
 * Author             : Jack Shaver
 * Date               : 4/11/2025
 * Description        : Internal ADC Header
 ********************************************************************************/
 
#ifndef adc_h
#define adc_h

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    batteryVoltage, pcbTemperature
} adcChannelType;

extern void adcInit();

// Single channel read
extern float adcRead(adcChannelType channel);

// Console Interface
extern void adcRegisterCommands();

#ifdef __cplusplus
}
#endif

#endif