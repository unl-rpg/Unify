/********************************************************************************
 * File Name          : i2c.h
 * Author             : Jack Shaver
 * Date               : 3/31/2025
 * Description        : I2C Bus Driver
 ********************************************************************************/
 
#ifndef i2c_h
#define i2c_h

#ifdef __cplusplus
extern "C" {
#endif

// Inputs
#define I2C_KEY 0x0001
#define I2C_FIRE_BUTTON 0x0002
#define I2C_POWER_BUTTON 0x0004
#define I2C_CHARGE_DETECT 0x0008
#define I2C_IGNITER_DETECT 0x0010
#define I2C_SD_CARD_DETECT 0x0020
#define I2C_USB_V_DETECT 0x0040
#define I2C_USB_I_DETECT 0x0080

// Outputs 
#define I2C_STATUS_LED 0x0101
#define I2C_ERROR_LED 0x0102
#define I2C_POWER_LED 0x0104
#define I2C_CHARGE_LED 0x0108
#define I2C_IGNITER_ENABLE 0x0110
#define I2C_SYSTEM_ENABLE 0x0120
#define I2C_POWER_OFF 0x0140
#define I2C_TWAI_ENABLE 0x0180

#include <stdio.h>
#include "driver/gpio.h"

extern void i2cInit(const gpio_num_t sclPin, const gpio_num_t sdaPin);
extern void i2cDeinit();

// Console Interface
extern void i2cRegisterCommands();

// Reading from i2c gpio 
extern bool i2cGetGpioSignal(uint16_t i2cMask);

// Writing to i2c gpio (only for output expander, no error if improperly used)
extern void i2cSetGpioSignal(uint16_t i2cMask, bool value);

// On an interrupt, this detects the signal which triggered
extern void i2cGetInterruptSources(uint8_t *diff, uint8_t *values);



#ifdef __cplusplus
}
#endif

#endif