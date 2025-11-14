#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/Queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "connection.h"
#include "store.h"
#include "gps.h"
#include "queue.h"
#include "driver/adc.h"

static const char* TAG = "MAIN";
#define MQ_ADC_CHANNEL ADC_CHANNEL_6


void mq_sensor_task(void *pvParameters)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(MQ_ADC_CHANNEL, ADC_ATTEN_DB_11);

    while (1)
    {
        int raw = adc1_get_raw(MQ_ADC_CHANNEL);
        char msg[32];
        snprintf(msg, sizeof(msg), "MQ6: %d", raw);

        if (xQueueSend(gpsMessageQueue, msg, 0) != pdTRUE)
        {
            ESP_LOGW(TAG, "Queue full, sensor data dropped");
        }
        ESP_LOGI(TAG, "%s", msg);

        vTaskDelay(pdMS_TO_TICKS(10000)); // every 10 seconds
    }
}

void gps_task(void *pvParameters)
{
   char msg[64];
    char lat[16], latDir[2], lon[16], lonDir[2];

    while (1)
    {
        const char *nmea = simulateGPS();
        parseLatLon(nmea, lat, latDir, lon, lonDir);

        snprintf(msg, sizeof(msg), "Lat:%s%s Lon:%s%s", lat, latDir, lon, lonDir);

        if (xQueueSend(gpsMessageQueue, msg, 0) != pdTRUE)
        {
            ESP_LOGW(TAG, "Queue full, GPS data dropped");
        }
        else
        {
            ESP_LOGI(TAG, "Queued GPS data: %s", msg);
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Run every 5 seconds
    }
        /*if (gps_read(&gps) && gps.valid)
        {
            char msg[64];
            snprintf(msg, sizeof(msg),
                     "Lat:%s%s Lon:%s%s",
                     gps.latitude, gps.lat_dir, gps.longitude, gps.lon_dir);

            if (xQueueSend(gpsMessageQueue, &msg, 0) != pdTRUE)
            {
                ESP_LOGW(TAG, "Queue full, message dropped");
            }
        }*/
}

void mqtt_publish_task(void *pvParameters) {
	char msg[64];
	while(mqtt_is_connected()) {
		if(xQueueReceive(gpsMessageQueue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE) {
			mqtt_publish_message("esp32/mqttsmgtest", msg);
            ESP_LOGI(TAG, "Published GPS message: %s", msg);
		}
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
	vTaskDelete(NULL);
}

void store_task(void *pvParameters)
{
    char msg[64];
    while (!mqtt_is_connected())
    {
        if (xQueueReceive(gpsMessageQueue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE)
        {
            store_message_in_nvs("esp32/mqttsmgtest", msg);
            ESP_LOGI(TAG, "Stored GPS message: %s", msg);
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    vTaskDelete(NULL);
}

static void simulate_network_event(void* arg)
{
    static bool connected = true;

    if (connected) {
        ESP_LOGW("SIM", "Simulating WiFi disconnection...");
        esp_wifi_disconnect();
        connected = false;
    } else {
        ESP_LOGW("SIM", "Simulating WiFi reconnection...");
        esp_wifi_connect();
        connected = true;
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
	message_store_init();
	message_queue_init();
	const esp_timer_create_args_t timer_args = {
        .callback = &simulate_network_event,
        .name = "simulate_net"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 30 * 1000000));
    xTaskCreate(gps_task, "gps_task", 4096, NULL, 5, NULL);
    xTaskCreate(mq_sensor_task, "MQ6 Sensor Task", 4096, NULL, 4, NULL);
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10000)); // Keep app alive
    }
}
