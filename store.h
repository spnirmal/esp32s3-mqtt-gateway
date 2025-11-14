#ifndef MESSAGE_STORE_H
#define MESSAGE_STORE_H

#include "esp_err.h"

// Initialize NVS for message storage
esp_err_t message_store_init(void);

// Store message (called when disconnected)
esp_err_t store_message_in_nvs(const char *topic, const char *message);

// Send all stored messages and clear them (called when MQTT reconnects)
void resend_stored_messages(void *mqtt_client);

#endif
