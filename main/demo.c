/* NMEA parsing example for ESP32.
 * Based on "parse_stdin.c" example from libnmea.
 * Copyright (c) 2015 Jack Engqvist Johansson.
 * Additions Copyright (c) 2017 Ivan Grokhotkov.
 * See "LICENSE" file in libnmea directory for license.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nmea.h"
#include "gpgll.h"
#include "gpgga.h"
#include "gprmc.h"
#include "gpgsa.h"
#include "gpvtg.h"
#include "gptxt.h"
#include "gpgsv.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "demo";

#include "websocket_server.h"

extern MessageBufferHandle_t xMessageBufferToClient;
extern EventGroupHandle_t xEventWebSocket;

void demo_task(void* pvParameters)
{
	ESP_LOGW(TAG, "Demonstration Mode");
	nmea_position longitude;
	nmea_position latitude;
	// This mode displays a map of the Statue of Liberty.
	longitude.degrees = -74;
	longitude.minutes = -2.7036;
	latitude.degrees = 40;
	latitude.minutes = 41.3892;

	while(1) {
		// Whether the browser has started or not
		// If the browser is not started, do not send the message.
		EventBits_t bits = xEventGroupGetBits(xEventWebSocket);
		ESP_LOGI(TAG, "bits=0x%x", bits);
		if (bits == 0x01) {
			// send current location to web client
			cJSON *request;
			request = cJSON_CreateObject();
			cJSON_AddStringToObject(request, "id", "nmea-request");
			cJSON_AddNumberToObject(request, "longitude_degrees", longitude.degrees);
			cJSON_AddNumberToObject(request, "longitude_minutes", longitude.minutes);
			cJSON_AddNumberToObject(request, "latitude_degrees", latitude.degrees);
			cJSON_AddNumberToObject(request, "latitude_minutes", latitude.minutes);
			cJSON_AddNumberToObject(request, "options", 1); // Use Fullscreen Control
			char *my_json_string = cJSON_Print(request);
			ESP_LOGD(TAG, "my_json_string\n%s",my_json_string);
			size_t xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, my_json_string, strlen(my_json_string), NULL);
			if (xBytesSent != strlen(my_json_string)) {
				ESP_LOGE(TAG, "xMessageBufferSend fail");
			}
			cJSON_Delete(request);
			cJSON_free(my_json_string);
		}
		vTaskDelay(100);
	}
	vTaskDelete(NULL);
}
