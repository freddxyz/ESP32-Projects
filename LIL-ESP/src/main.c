#include <stdio.h>

#include "esp_spiffs.h"
#include "esp_log.h"

#include "lil.h"

const char* TAG = "ESP-LIL TEST";

int get_file_size(FILE *fstream) 
{
    int rtn = 0;
    fseek(fstream, 0, SEEK_END);
    rtn = ftell(fstream);
    fseek(fstream, 0, SEEK_SET);
    return rtn;
}

esp_vfs_spiffs_conf_t spiffs_conf = {
    .base_path = "/storage",
    .partition_label = "storage",
    .max_files = 5,
    .format_if_mount_failed = false
};

esp_err_t mount_storage(void)
{
    esp_err_t rtn = esp_vfs_spiffs_register(&spiffs_conf);

    if (rtn != ESP_OK) {
        if (rtn == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount the SPIFFS");
        } else if (rtn == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find the SPIFFS partition.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the SPIFFS partition: %s", esp_err_to_name(rtn));
        }
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Storage mounted successfully.");

    size_t total, used;
    ESP_ERROR_CHECK(esp_spiffs_info(spiffs_conf.partition_label, &total, &used));

    ESP_LOGI(TAG, "Total: %d, Used: %d", total, used);
    
    return ESP_OK;
}


void app_main() 
{
    mount_storage();
    FILE *lil_main_filestream = fopen("storage/main.lil", "r");
    //get size of file
    int lil_main_filesize = get_file_size(lil_main_filestream);
    char *buffer = (char*) malloc(lil_main_filesize + 1);
    ESP_LOGI(TAG, "Amount expected: %d, amount read: %d", lil_main_filesize, fread(buffer, 1, lil_main_filesize, lil_main_filestream));
    ESP_LOGI(TAG, "%s", buffer);

}