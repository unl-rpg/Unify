/********************************************************************************
 * File Name          : espnow.c
 * Author             : Jack Shaver
 * Date               : 4/12/2025
 * Description        : Espnow Source
 ********************************************************************************/

#include "espnow.h"
 
// Good to have
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "esp_wifi.h"
#include "esp_now.h"
//#include "nvs_flash.h"

#define WIFI_CHANNEL 12
#define MAC_LENGTH 6


static uint8_t peerAddress[6];

SemaphoreHandle_t espnowSemaphore = NULL; 


static void wifiInit(void){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE));
 
    // Enable high speed mode
    //esp_wifi_internal_set_fix_rate(ESP_IF_WIFI_STA, 1, WIFI_PHY_RATE_MCS7_SGI);
}


void espnowInit(uint8_t remoteAddress[6]){
    memcpy(&peerAddress, remoteAddress, MAC_LENGTH);
	
	espnowSemaphore = xSemaphoreCreateBinary();    
	xSemaphoreGive(espnowSemaphore);
    
    wifiInit();
    ESP_ERROR_CHECK(esp_now_init());

    esp_now_peer_info_t peer = {
        .lmk = {0},
        .channel = WIFI_CHANNEL, // ranges from 0-14
        .ifidx = ESP_IF_WIFI_STA,
        .encrypt = false,
        .priv = NULL,
    };
    memcpy(&peer.peer_addr, peerAddress, MAC_LENGTH);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
}

void espnowGetMAC(uint8_t localAddress[6]){
	ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, localAddress));
}

void espnowRegisterRecieveCallback(esp_now_recv_cb_t recieveCallback){
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recieveCallback));
}

void espnowTransmit(uint8_t *message, int len){
	
	if(xSemaphoreTake(espnowSemaphore, 0xffff) == pdTRUE ){
		ESP_ERROR_CHECK(esp_now_send(peerAddress, message, len * sizeof(message)));
		xSemaphoreGive(espnowSemaphore); 
    }	
    
}

void espnowSendCommand(uint8_t cmd){
	uint8_t command[1] = {cmd};
	espnowTransmit(command, 1);
}