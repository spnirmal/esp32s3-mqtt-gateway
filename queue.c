#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "queue.h"


QueueHandle_t gpsMessageQueue = NULL;

void message_queue_init(void)
{
    gpsMessageQueue = xQueueCreate(10, sizeof(char[64])); // Queue for up to 10 GPS messages
}
