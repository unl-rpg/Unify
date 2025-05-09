/********************************************************************************
 * File Name          : sd.c
 * Author             : Jack Shaver
 * Date               : 4/11/2025
 * Description        : SD Card SDMMC source
 ********************************************************************************/
 
#include "sd.h"

#include "pins.h"

#include <string.h>
//#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h" // sdmmc print info
#include "driver/sdmmc_host.h"

#include "esp_log.h"




#define MAX_CHAR_SIZE 64
#define MOUNT_POINT "/unify"


static sdmmc_card_t *card;

static const char *TAG = "sdcard";


void sdInit(){
	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = true,
		.max_files = 5, // may want to increase
		.allocation_unit_size = 16 * 1024 // check
	};
	
	sdmmc_host_t host = SDMMC_HOST_DEFAULT(); // should be 20MHz
	//host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // untested
	host.slot = SDMMC_HOST_SLOT_0;
	//host.max_freq_khz = SDMMC_FREQ_SDR50; // untested
	host.flags &= ~SDMMC_HOST_FLAG_DDR; // not ddr flag
	
	
	sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
	slot.clk = SD_CARD_CLK_PIN;
	slot.cmd = SD_CARD_CMD_PIN;
	slot.d0 = SD_CARD_D0_PIN;
	slot.d1 = SD_CARD_D1_PIN;
	slot.d2 = SD_CARD_D2_PIN;
	slot.d3 = SD_CARD_D3_PIN;
	slot.width = 4;
	
	const char mount_point[] = MOUNT_POINT;
	esp_err_t ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot, &mount_config, &card); 
	
	if(ret != ESP_OK){
		if(ret == ESP_FAIL){
			ESP_LOGE(TAG, "Failed to mount filesystem. " "If you want the card to be formatted, change the mount_config option to true.");
		}else{
			ESP_LOGE(TAG, "Failed to initialize the card (%s). " "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
		}
		return;
	}
	
	//sdmmc_card_print_info(stdout, card);
}

static esp_err_t writeFile(const char *path, char *data){
	ESP_LOGI(TAG, "Opening file %s", path);
	FILE *f = fopen(path, "w");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for writing");
		return ESP_FAIL;
	}
	fprintf(f, data);
	fclose(f);
	ESP_LOGI(TAG, "File written");
	
	return ESP_OK;
}

static esp_err_t readFile(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}

bool file_exists(char *filename){
	struct stat buffer;
	return (stat (filename, &buffer) == 0);
}

// This function creates, dumps and closes a new file.
// Increments the file name if it exists already
void sdCreateFile(char* filename, long *timeStamp, uint16_t *data, long samples){
	int counter = 0;
	char filePath[50];
	memset(filePath, 0, sizeof(filePath));
	do{
		sprintf(filePath, "%s/%s%d.csv", MOUNT_POINT, filename, counter);
		counter++;
	}while(file_exists(filePath));
	
	FILE *f = fopen(filePath, "w");
	if(f == NULL){
		ESP_LOGE(TAG, "Failed to open file for writing");
		return;
	}
	for(int i = 0; i < samples; i++){
		fprintf(f, "%ld, %d\n", timeStamp[i], data[i]);
	}
	fclose(f);
}