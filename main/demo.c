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
			// Send current location to web client
			cJSON *request;
			request = cJSON_CreateObject();
			cJSON_AddStringToObject(request, "id", "nmea-request");
			cJSON_AddNumberToObject(request, "longitude_degrees", longitude.degrees);
			cJSON_AddNumberToObject(request, "longitude_minutes", longitude.minutes);
			cJSON_AddNumberToObject(request, "latitude_degrees", latitude.degrees);
			cJSON_AddNumberToObject(request, "latitude_minutes", latitude.minutes);
			cJSON_AddNumberToObject(request, "zoom_level", 0);
			cJSON_AddNumberToObject(request, "options", 0);
			char *nmea_string = cJSON_Print(request);
			ESP_LOGD(TAG, "nmea_string\n%s",nmea_string);
			size_t xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, nmea_string, strlen(nmea_string), NULL);
			if (xBytesSent != strlen(nmea_string)) {
				ESP_LOGE(TAG, "xMessageBufferSend fail");
			}
			cJSON_Delete(request);
			cJSON_free(nmea_string);

			// Send zoomcontrol disable to web client
			request = cJSON_CreateObject();
			cJSON_AddStringToObject(request, "id", "zoomcontrol-request");
			cJSON_AddNumberToObject(request, "zoom_control", 0);
			char *zoom_control_disable_string = cJSON_Print(request);
			ESP_LOGD(TAG, "zoom_control_disable_string\n%s",zoom_control_disable_string);
			xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, zoom_control_disable_string, strlen(zoom_control_disable_string), NULL);
			if (xBytesSent != strlen(zoom_control_disable_string)) {
				ESP_LOGE(TAG, "xMessageBufferSend fail");
			}
			cJSON_Delete(request);
			cJSON_free(zoom_control_disable_string);

			// Send zoomlevel to web client
			for(int zoom_level=0;zoom_level<17;zoom_level++) {
				request = cJSON_CreateObject();
				cJSON_AddStringToObject(request, "id", "zoomlevel-request");
				cJSON_AddNumberToObject(request, "zoom_level", zoom_level);
				char *zoom_level_string = cJSON_Print(request);
				ESP_LOGD(TAG, "zoom_level_string\n%s",zoom_level_string);
				xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, zoom_level_string, strlen(zoom_level_string), NULL);
				if (xBytesSent != strlen(zoom_level_string)) {
					ESP_LOGE(TAG, "xMessageBufferSend fail");
				}
				cJSON_Delete(request);
				cJSON_free(zoom_level_string);
				vTaskDelay(100);
			}

			// Send zoomcontrol enable to web client
			request = cJSON_CreateObject();
			cJSON_AddStringToObject(request, "id", "zoomcontrol-request");
			cJSON_AddNumberToObject(request, "zoom_control", 1);
			char *zoom_control_enable_string = cJSON_Print(request);
			ESP_LOGD(TAG, "zoom_control_enable_string\n%s",zoom_control_enable_string);
			xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, zoom_control_enable_string, strlen(zoom_control_enable_string), NULL);
			if (xBytesSent != strlen(zoom_control_enable_string)) {
				ESP_LOGE(TAG, "xMessageBufferSend fail");
			}
			cJSON_Delete(request);
			cJSON_free(zoom_control_enable_string);

			// Send fullscreen to web client
			request = cJSON_CreateObject();
			cJSON_AddStringToObject(request, "id", "fullscreen-request");
			cJSON_AddNumberToObject(request, "fullscreen", 1);
			char *fullscreen_string = cJSON_Print(request);
			ESP_LOGD(TAG, "fullscreen_string\n%s",fullscreen_string);
			xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, fullscreen_string, strlen(fullscreen_string), NULL);
			if (xBytesSent != strlen(fullscreen_string)) {
				ESP_LOGE(TAG, "xMessageBufferSend fail");
			}
			cJSON_Delete(request);
			cJSON_free(fullscreen_string);

			// Send message to web client
			request = cJSON_CreateObject();
			cJSON_AddStringToObject(request, "id", "message-request");
			cJSON_AddStringToObject(request, "position", "bottomright");
			cJSON_AddNumberToObject(request, "timeout", 5000);
			cJSON_AddStringToObject(request, "message", "<p><a href=\"https://github.com/nopnop2002/esp-idf-leaflet\">GitHub esp-idf-leaflet</a></p>");
			char *message_string = cJSON_Print(request);
			ESP_LOGD(TAG, "message_string\n%s",message_string);
			xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, message_string, strlen(message_string), NULL);
			if (xBytesSent != strlen(message_string)) {
				ESP_LOGE(TAG, "xMessageBufferSend fail");
			}
			cJSON_Delete(request);
			cJSON_free(message_string);
		}
		vTaskDelay(100);
	}
	vTaskDelete(NULL);
}
