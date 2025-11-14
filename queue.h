#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t gpsMessageQueue;

void message_queue_init(void);
