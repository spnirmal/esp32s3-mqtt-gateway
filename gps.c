#include "gps.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

//#define GPS_UART UART_NUM_1
//#define GPS_RX_PIN 18
//#define GPS_TX_PIN 17

static const char *TAG = "GPS";

static const char *simulatedNMEA[] = {
    "$GPRMC,123519,A,2807.038,N,07702.320,E,0.5,054.7,131120,0.0,W*70",
    "$GPRMC,123520,A,2807.038,N,07702.321,E,0.6,054.7,131120,0.0,W*71",
    "$GPRMC,123521,A,2807.039,N,07702.322,E,0.7,054.7,131120,0.0,W*72"
};
#define NUM_SIMULATED 3

extern QueueHandle_t gpsMessageQueue;

const char *simulateGPS(void)
{
    static int idx = 0;
    const char *nmea = simulatedNMEA[idx % NUM_SIMULATED];
    idx++;
    return nmea;
}

void parseLatLon(const char *nmea, char *lat, char *latDir, char *lon, char *lonDir)
{
    char nmeaCopy[100];
    strncpy(nmeaCopy, nmea, sizeof(nmeaCopy));
    nmeaCopy[sizeof(nmeaCopy) - 1] = '\0';

    char *token = strtok(nmeaCopy, ",");
    int field = 0;

    while (token != NULL)
    {
        field++;
        if (field == 4) strcpy(lat, token);
        if (field == 5) strcpy(latDir, token);
        if (field == 6) strcpy(lon, token);
        if (field == 7) strcpy(lonDir, token);
        token = strtok(NULL, ",");
    }
}
/*commented out due to unreliable connection indoor used simulated data*/
/*void gps_init(void)
{
    uart_config_t config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(GPS_UART, &config);
    uart_set_pin(GPS_UART, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(GPS_UART, 2048, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "GPS UART initialized");
}

// Parse a line if it's GPRMC and fill gps_data_t
static bool parse_gprmc(const char *line, gps_data_t *out)
{
    if (strncmp(line, "$GPRMC", 6) != 0)
        return false;

    char buf[128];
    strncpy(buf, line, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';

    char *token = strtok(buf, ",");
    int field = 0;
    bool valid = false;

    while (token != NULL)
    {
        field++;
        switch (field)
        {
        case 3:
            strncpy(out->latitude, token, sizeof(out->latitude));
            break;
        case 4:
            strncpy(out->lat_dir, token, sizeof(out->lat_dir));
            break;
        case 5:
            strncpy(out->longitude, token, sizeof(out->longitude));
            break;
        case 6:
            strncpy(out->lon_dir, token, sizeof(out->lon_dir));
            valid = true;
            break;
        }
        token = strtok(NULL, ",");
    }

    out->valid = valid;
    return valid;
}

bool gps_read(gps_data_t *data)
{
    static char line[128];
    static int pos = 0;
    char c;
    int len = uart_read_bytes(GPS_UART, (uint8_t *)&c, 1, 100 / portTICK_PERIOD_MS);

    if (len > 0)
    {
        if (c == '\n')
        {
            line[pos] = '\0';
            pos = 0;

            if (parse_gprmc(line, data))
            {
                ESP_LOGI(TAG, "Lat:%s%s Lon:%s%s", data->latitude, data->lat_dir,
                         data->longitude, data->lon_dir);
                return true;
            }
        }
        else if (pos < sizeof(line) - 1)
        {
            line[pos++] = c;
        }
    }
    return false;
}*/
