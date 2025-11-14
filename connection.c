#include "connection.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "stdbool.h"
#include "store.h"

#define WIFI_SSID "SECONDFLOOR"
#define WIFI_PASS "12345678"
static const char *TAG = "MQTT";
static bool mqtt_connected = false;
static bool mqtt_started = false;
static esp_mqtt_client_handle_t mqtt_client = NULL;

TaskHandle_t mqttPublishTaskHandle = NULL;
TaskHandle_t storeTaskHandle = NULL;

bool mqtt_is_connected(void)
{
    return mqtt_connected;
}

int mqtt_publish_message(const char *topic, const char *message)
{
    if (!mqtt_client)
        return -1;

    return esp_mqtt_client_publish(mqtt_client, topic, message, 0, 1, 0);
    
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
		
		case MQTT_EVENT_CONNECTED:
    		ESP_LOGI(TAG, "Connected to MQTT broker!");
   			mqtt_connected = true;
   			
			resend_stored_messages(client);
			
    		if (mqttPublishTaskHandle == NULL) {
        	xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 5, &mqttPublishTaskHandle);
    		}
    		break;

        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;  
            ESP_LOGW(TAG, "MQTT Disconnected");
            
            if (mqttPublishTaskHandle != NULL) {
       			 vTaskDelete(mqttPublishTaskHandle);
        		 mqttPublishTaskHandle = NULL;
    		}
    		xTaskCreate(store_task, "store_task", 4096, NULL, 5, &storeTaskHandle);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "Message published successfully, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT Error occurred");
            break;

        default:
            ESP_LOGD(TAG, "Other event id: %d", event->event_id);
            break;
    }
}


void mqtt_start(void)
{
    if (mqtt_started) {
        ESP_LOGI(TAG, "mqtt already started");
        return;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://broker.hivemq.com:1883",
        
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!mqtt_client) {
        ESP_LOGE(TAG, "Failed to init mqtt client");
        return;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    mqtt_started = true;
    ESP_LOGI(TAG, "mqtt_start requested");
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi connecting...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        mqtt_start(); // once IP ready connect to MQTT
    }
}

void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &wifi_event_handler,
                                               NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                               IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler,
                                               NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi start requested");
}