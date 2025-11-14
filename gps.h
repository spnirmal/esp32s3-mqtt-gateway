#ifndef GPS_H
#define GPS_H

#include "esp_err.h"
#include <stdbool.h>


//void gps_init(void);
//bool gps_read(gps_data_t *data);
const char* simulateGPS(void);
void parseLatLon(const char *nmea, char *lat, char *latDir, char *lon, char *lonDir);

#endif
