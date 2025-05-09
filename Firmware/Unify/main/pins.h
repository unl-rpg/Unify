/********************************************************************************
 * File Name          : pins.h
 * Author             : Jack Shaver
 * Date               : 4/1/2025
 * Description        : Definitions of IO Pins
 ********************************************************************************/

#ifndef pins_h
#define pins_h
 
#ifdef __cplusplus
extern "C" {
#endif

// GPIO Pins Definitions
#define BUTTON_PIN 0
#define CONFIG_PIN 18
#define BUZZER_PIN 45
#define BLINK_PIN 46

// ADC Pins Definitions
#define PCB_TEMP_ADC_PIN 1
#define BATTERY_VOLT_ADC_PIN 2

// SD Card Pins Definitions
#define SD_CARD_D0_PIN 47
#define SD_CARD_D1_PIN 48
#define SD_CARD_D2_PIN 3
#define SD_CARD_D3_PIN 9
#define SD_CARD_CMD_PIN 14
#define SD_CARD_CLK_PIN 21

// I2C Pins Definitions
#define I2C_INTERRUPT_PIN 40
#define I2C_SDA_PIN 41
#define I2C_SCL_PIN 42

// SPI Pins Definitions
// NOTE THAT THERE WAS AN ERROR WITH THE SCHEMATIC
// had to swap clk and cs here
#define SPI_CS_PIN 12
#define SPI_MOSI_PIN 11
#define SPI_CLK_PIN 10
#define SPI_MISO_PIN 13

// TWAI Pins Definitions
#define TWAI_TX_PIN 38
#define TWAI_RX_PIN 39

// Expansion Pins Definitions
#define EXP_0_PIN 4
#define EXP_1_PIN 5
#define EXP_2_PIN 6
#define EXP_3_PIN 7
#define EXP_4_PIN 15
#define EXP_5_PIN 16
#define EXP_6_PIN 17
#define EXP_7_PIN 8

#ifdef __cplusplus
}
#endif
 
#endif 
 