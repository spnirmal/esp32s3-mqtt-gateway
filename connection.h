#pragma once

#include "esp_err.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void wifi_init(void);
void mqtt_start(void);
bool mqtt_is_connected(void);
int mqtt_publish_message(const char *topic, const char *message);
void mqtt_publish_task(void *pvParameters);
extern TaskHandle_t mqttPublishTaskHandle;
void store_task(void *pvParameters);
extern TaskHandle_t storeTaskHandle;