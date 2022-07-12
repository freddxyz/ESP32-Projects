#pragma once

/*#include "esp_err.h"
#include "http_server.h"*/

#include <stdio.h>
#include <sys/unistd.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "ff.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "esp_ota_ops.h"
#include "ota_updater.h"

esp_err_t mount_storage(void);
httpd_handle_t start_http_server(void);
void wifi_sta_init(void);

extern char* TAG;