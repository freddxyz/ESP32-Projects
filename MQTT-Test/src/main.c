#include "nvs_flash.h"
#include "mqtt_client.h"
#include "mqtt_test.h"

char *TAG = "MQTT TEST";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    int msg_id;
    switch((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to broker!");
            msg_id = esp_mqtt_client_publish(client, "topic/test", "did you get this?", 0, 1, 0);
            ESP_LOGI(TAG, "Published message!");
            ESP_LOGI(TAG, "%d", msg_id);
            msg_id = esp_mqtt_client_subscribe(client, "topic/test", 1);
            ESP_LOGI(TAG, "Subscribed");
            ESP_LOGI(TAG, "%d", msg_id);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Subscribed successfully!");
            msg_id = esp_mqtt_client_publish(client, "topic/test", "I just connected!", 0, 1, 0);
            ESP_LOGI(TAG, "%d", msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Got published message:");
            ESP_LOGI(TAG, "Topic: %s", event->topic);
            ESP_LOGI(TAG, "Data: %s", event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG,"reported from esp-tls: %s", esp_err_to_name(event->error_handle->esp_tls_last_esp_err));
                ESP_LOGI(TAG,"reported from tls stack: %d", event->error_handle->esp_tls_stack_err);
                ESP_LOGI(TAG, "captured as transport's socket errno: %d",  event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d - unhandled event", event->event_id);
            break;
    }
}

esp_err_t mqtt_start(void) 
{
    esp_mqtt_client_config_t config = {
        .uri = "mqtt://192.168.1.76:1883"
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "Started mqtt");

    return ESP_OK;
}
void app_main() 
{
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    esp_err_t rtrn = nvs_flash_init();
    
    if (rtrn == ESP_ERR_NVS_NO_FREE_PAGES) {
        nvs_flash_erase();
        rtrn = nvs_flash_init();
    }

    ESP_ERROR_CHECK(rtrn);

    wifi_sta_init();
    mqtt_start();   
}