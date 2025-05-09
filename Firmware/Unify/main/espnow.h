/********************************************************************************
 * File Name          : espnow.h
 * Author             : Jack Shaver
 * Date               : 4/12/2025
 * Description        : ESPNOW Header
 ********************************************************************************/
 
#ifndef espnow_h
#define espnow_h

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_now.h"
#include <string.h> //memcpy

// Think of a better way to do this and name them
// Maybe commands and callbacks, idk
#define espnowAcknowledgeCommand 0x00
#define espnowPingCommand 0x01
#define espnowFireCommand 0x02
#define espnowAbortCommand 0x03

#define espnowUnrecognizedCommand 0x10
#define espnowAbortConfirmationCommand 0x11
#define espnowBadKeyStateCommand 0x12
#define espnowNoSdCardCommand 0x13
#define espnowBadIgniterCommand 0x14
#define espnowGoodFireCommand 0x15
#define espnowBadFireCommand 0x16
#define espnowConfirmCountdown 0x17

extern void espnowInit(uint8_t remoteAddress[6]);
extern void espnowGetMAC(uint8_t localAddress[6]);
extern void espnowRegisterRecieveCallback(esp_now_recv_cb_t recieveCallback);
//extern void espnowRegisterTransmitCallback(esp_now_send_cb_t transmitCallback);

extern void espnowTransmit(uint8_t *message, int len);

extern void espnowSendCommand(uint8_t cmd);

#ifdef __cplusplus
}
#endif

#endif