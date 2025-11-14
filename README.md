# ESP32-S3 Telemetry Node (GPS + MQ6 + MQTT + Offline Storage)

a small embedded demo built on the **ESP32-S3 Wroom-1 DevKit**. The goal is to reliably publish sensor data (GPS + MQ6) to an MQTT server, even when the device loses Wi-Fi or internet.

The system buffers all sensor readings in **NVS flash** every 10 seconds, and whenever connectivity is available it pushes pending messages in FIFO order.

This implementation uses **FreeRTOS**, queues, tasks, and MQTT events for synchronization.

## Features Implemented

### 1. GNSS Data Fetching

* GPS module over UART (NMEA sentences)
* Extracts latitude & longitude
* Runs in its own FreeRTOS task (`gps_task`)
* But due to unreliable connection indoors i have switched to a simulated data for debugging and prototyping.

### 2. MQ6 Gas Sensor Data Fetching

* MQ6 connected to **ADC1 channel 6 (GPIO7)**
* Periodic ADC sampling
* Runs in `mq6_task`

### 3. MQTT Communication

* ESP-IDF MQTT client
* Auto reconnect
* Publishes only when connected
* Controlled via `mqtt_event_handler`

### 4. Local Flash Storage (NVS)

* Stores telemetry every 10 seconds
* Uses sequential keys with read/write pointers
* Survives reset & power loss

### 5. Offline Sync Logic

* If Wi-Fi or MQTT is down → only store data
* When back online → syncs oldest entries first

## Design Decisions

* **NVS over SPIFFS:** simpler, predictable, lower overhead
* **Separate sensor tasks:** no blocking, deterministic timing
* **Sync task disabled until MQTT connected:** prevents race conditions

## Current Limitations

* Tasks created and deleted for every MQTT connect and disconnect . this can cause fragmentation alternatively could have used tasksuspend, taskresume.
* NVS can fill up if connectivity is down for long
* Minimal GPS parsing
* MQ6 readings not calibrated

![output](https://github.com/spnirmal/esp32s3-mqtt-gateway/blob/main/resource/image.png)
here the GPS and MQ6 readings are stored in nvs and once MQTT reconnected resent again.
