#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "lil_esp_general.h"

EventGroupHandle_t wifi_event_group;

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

            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "Station connected to access point.");
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

void wifi_softap_init(void)
{
    esp_netif_ip_info_t ip_info;

    IP4_ADDR(
        &ip_info.gw,
        192,
        168,
        1,
        1
    );

    IP4_ADDR(
        &ip_info.ip,
        192,
        186,
        1,
        1
    );

    IP4_ADDR(
        &ip_info.netmask,
        255,
        255,
        255,
        0
    );

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *ap_handle = esp_netif_create_default_wifi_ap();
    esp_netif_dhcps_stop(ap_handle);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_handle, &ip_info));
    esp_netif_dhcps_start(ap_handle);

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    esp_event_handler_instance_t instance_wifi_any_id;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, 
        ESP_EVENT_ANY_ID, 
        &on_wifi_event, 
        NULL, 
        &instance_wifi_any_id
    ));
    
    //Configuration with ssid and password for network 
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "LIL-ESP AP",
            .password = "12345678",
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA2_PSK
        }
    };

    esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
    ESP_ERROR_CHECK(esp_wifi_start());
    

}