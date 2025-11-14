#include "store.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>

static const char *TAG = "MSG_STORE";

esp_err_t message_store_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

esp_err_t store_message_in_nvs(const char *topic, const char *message)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("msg_storage", NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return err;

    // Find the next available message index
    uint32_t msg_count = 0;
    nvs_get_u32(handle, "count", &msg_count);
    msg_count++;

    char key[16];
    snprintf(key, sizeof(key), "msg_%lu", msg_count);

    // Combine topic + payload
    char stored[256];
    snprintf(stored, sizeof(stored), "%s|%s", topic, message);

    err = nvs_set_str(handle, key, stored);
    if (err == ESP_OK)
    {
        nvs_set_u32(handle, "count", msg_count);
        nvs_commit(handle);
        ESP_LOGI(TAG, "Stored message #%lu", msg_count);
    }
    nvs_close(handle);
    return err;
}

void resend_stored_messages(void *mqtt_client)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open("msg_storage", NVS_READWRITE, &handle);
    if (err != ESP_OK)
        return;

    uint32_t msg_count = 0;
    nvs_get_u32(handle, "count", &msg_count);

    for (uint32_t i = 1; i <= msg_count; i++)
    {
        char key[16];
        snprintf(key, sizeof(key), "msg_%lu", i);

        char stored[256];
        size_t len = sizeof(stored);
        if (nvs_get_str(handle, key, stored, &len) == ESP_OK)
        {
            char *sep = strchr(stored, '|');
            if (sep)
            {
                *sep = '\0';
                const char *topic = stored;
                const char *payload = sep + 1;

                int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
                ESP_LOGI(TAG, "Resent msg_id=%d, topic=%s", msg_id, topic);
            }
            nvs_erase_key(handle, key); // delete after sending
        }
    }

    // Reset count
    nvs_set_u32(handle, "count", 0);
    nvs_commit(handle);
    nvs_close(handle);
}
