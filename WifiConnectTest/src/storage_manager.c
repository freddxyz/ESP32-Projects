// #include "esp_err.h"
// #include "esp_vfs.h"
// #include "esp_spiffs.h"
// #include "esp_log.h"
 #include "ota_updater.h"

esp_vfs_spiffs_conf_t spiffs_conf = {
    .base_path = "/storage",
    .partition_label = "storage",
    .max_files = 5,
    .format_if_mount_failed = true
};

esp_err_t mount_storage(void) 
{
    esp_err_t rtrn = esp_vfs_spiffs_register(&spiffs_conf);

    if (rtrn != ESP_OK) {
        if (rtrn == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount the SPIFFS");
        } else if (rtrn == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find the SPIFFS partition.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the SPIFFS partition: %s", esp_err_to_name(rtrn));
        }
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Storage mounted successfully.");

    size_t total, used;
    ESP_ERROR_CHECK(esp_spiffs_info(spiffs_conf.partition_label, &total, &used));

    ESP_LOGI(TAG, "Total: %d, Used: %d", total, used);
    
    return ESP_OK;
}
