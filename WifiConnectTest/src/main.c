// #include <stdio.h>
// #include <sys/unistd.h>
// #include "esp_vfs.h"
// #include "esp_vfs_fat.h"
// #include "ff.h"
// #include "esp_spiffs.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/event_groups.h"
// #include "esp_event.h"
// #include "esp_wifi.h"
// #include "esp_http_server.h"
// #include "esp_log.h"
// #include "nvs_flash.h"
// #include "driver/gpio.h"
// #include "esp_ota_ops.h"
#include "ota_updater.h"

#define BUILT_IN_LED 13

char* TAG = "OTA UPDATER";

//xSemaphoreHandle connectionSemaphore;

//Event group that holds the bits for a successful or failed connection
/*EventGroupHandle_t wifi_event_group;

esp_vfs_spiffs_conf_t spiffs_conf = {
    .base_path = "/storage",
    .partition_label = "storage",
    .max_files = 5,
    .format_if_mount_failed = true
};

static void on_wifi_event(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    if (base == WIFI_EVENT) {
        switch(id){
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                xEventGroupSetBits(wifi_event_group, BIT1);
                ESP_LOGI(TAG, "Could not connect to WIFI.");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Connecting...");
                break;
            default:
                break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *got_ip_event = (ip_event_got_ip_t*) event_data;
        xEventGroupSetBits(wifi_event_group, BIT0);
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&got_ip_event->ip_info.ip));
    }
}

void wifi_sta_init(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    esp_event_handler_instance_t instance_wifi_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, 
        ESP_EVENT_ANY_ID, 
        &on_wifi_event, 
        NULL, 
        &instance_wifi_any_id
    ));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, 
        IP_EVENT_STA_GOT_IP,
        &on_wifi_event,
        NULL,
        &instance_got_ip
    ));
    
    //Configuration with ssid and password for network 
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "FredIotNetwork",
            .password = "12345678"
        }
    };

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    ESP_ERROR_CHECK(esp_wifi_start());
    
    //Wait for successful or failed connection 
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group, 
        BIT0 | BIT1, 
        pdFALSE, 
        pdFALSE, 
        portMAX_DELAY 
    );

    if (bits & BIT0) {    //BIT0 Connection Successful bit
        ESP_LOGI(TAG, "Connected successfully");
    } else if (bits & BIT1) {    //BIT1 Connection Fail bit
        ESP_LOGI(TAG, "Failed to connect.");
    } else {    //Ran out of time waiting for bits
        ESP_LOGI(TAG, "Connection attempt timed out.");
    }   
}

esp_err_t http_get_file_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    FILE *filestream;
    filestream = fopen("/storage/index.html", "r");
    char chunk[1024];
    size_t chunksize;
    const char* resp = "Unable to open file!";

    if (!filestream) {
        gpio_set_level(BUILT_IN_LED, 1);
        httpd_resp_send(req, resp, strlen(resp));
        ESP_LOGE(TAG, "Unable open file!");
        return ESP_FAIL;
    }
    do {
        chunksize = fread(chunk, 1, 1024, filestream);
        if (chunksize > 0 ) {
            httpd_resp_send_chunk(req, chunk, chunksize);
        }
    } while (chunksize != 0);

    fclose(filestream);

    return ESP_OK;
}

esp_err_t http_post_ota_update_handler(httpd_req_t *req) 
{
    const esp_partition_t *update_partition;
    update_partition = esp_ota_get_next_update_partition(NULL);

    esp_ota_handle_t update_handle;
    esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);

    char read_buffer[1024];
    
    int data_remaining = req->content_len;
    
    while (data_remaining > 0) {
        int data_read = httpd_req_recv(req, read_buffer, MIN(data_remaining, 1024));
        esp_ota_write(update_handle, (const void*)read_buffer, data_read);

        data_remaining -= data_read;
    }

    esp_ota_end(update_handle);
    esp_ota_set_boot_partition(update_partition);
    esp_restart();
}

httpd_uri_t uri_get = {
    .uri = "",
    .handler = http_get_file_handler,
    .method = HTTP_GET,
    .user_ctx = NULL
};

httpd_uri_t uri_ota_post = {
    .uri = "/ota_upload",
    .handler = http_post_ota_update_handler,
    .method = HTTP_POST,
    .user_ctx = NULL
};

//HTTP SERVER CODE
httpd_handle_t start_http_server(void)
{
    httpd_handle_t server = NULL;

    httpd_config_t server_config = HTTPD_DEFAULT_CONFIG();
    server_config.uri_match_fn = httpd_uri_match_wildcard;
    if (httpd_start(&server, &server_config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
    }

    return server;
}

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
    return ESP_OK;
}*/

// esp_err_t test_read_and_write_file(void) 
// {
//     FILE *test_file;

//     ESP_LOGI(TAG, "Trying to open file...");
    
//     test_file = fopen("/storage/hello.txt", "a");

//     ESP_LOGI(TAG, "what's going on?!!");

//     if (!test_file) {
//         ESP_LOGE(TAG, "Unable to create / write to file on SPIFFS partition on the flash: File not opening");
//         return ESP_FAIL;
//     }

//     ESP_LOGI(TAG, "trying to write to file...");
//     fprintf(test_file, "%s", "HELLO WORLD");
//     ESP_LOGI(TAG, "Wrote to file successfully!");
//     fclose(test_file);

//     //Test reading file

//     test_file = fopen("/storage/hello.txt", "r");

//     if (!test_file) {
//         ESP_LOGE(TAG, "Unable to read from file created on SPIFFS partition: file not opening");
//         return ESP_FAIL;
//     }

//     char data[1000];

//     fread(&data, 1, 100, test_file);

//     fclose(test_file);

//     ESP_LOGI(TAG, "Data from file: %s", data);

//     ESP_LOGI(TAG, "Tryin to open index.html...");

//     test_file = fopen("/storage/index.html", "r");

//     if (!test_file) {
//         ESP_LOGE(TAG, "Unable to open from index.html");
//         return ESP_FAIL;
//     }

//     fread(&data, 1, 1000, test_file);

//     fclose(test_file);

//     ESP_LOGI(TAG, "Data read from index.html: %s", data);

//     return ESP_OK;
// }

void app_main(void) 
{
    vTaskDelay(5000/portTICK_PERIOD_MS);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    esp_err_t rtrn = nvs_flash_init();      // Initialize flash memory and store the return value

    if (rtrn == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        rtrn = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(rtrn);

    ESP_ERROR_CHECK(mount_storage()); // mount flash storage
    //ESP_ERROR_CHECK(test_read_and_write_file()); // test reading and writing to spiffs flash partition 

    //size_t total, used;
    //ESP_ERROR_CHECK(esp_spiffs_info(spiffs_conf.partition_label, &total, &used));
   
    //ESP_LOGI(TAG, "SPIFFS Partition info: Total: %d, Used: %d", total, used);

    gpio_pad_select_gpio(BUILT_IN_LED);
    gpio_set_direction(BUILT_IN_LED, GPIO_MODE_OUTPUT);

    wifi_sta_init();

    //Start HTTP server
    start_http_server();
    ESP_LOGI(TAG, "IT WORKED!!!!!!");
}