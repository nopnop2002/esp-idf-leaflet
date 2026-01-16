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

static const char *TAG = "move";

#include "websocket_server.h"

extern MessageBufferHandle_t xMessageBufferToClient;
extern EventGroupHandle_t xEventWebSocket;

#define STEPS 20

void move_task(void* pvParameters)
{
	ESP_LOGW(TAG, "Move Mode");
	// This mode traverses Central Park.
	// We can get the location on Google Maps.
	// Starting Latitude: 40.76631
	// Starting Longitude: -73.97759
	nmea_position startLatitude;
	nmea_position startLongitude;
	startLatitude.degrees = 40;
	startLatitude.minutes = 0.76631*60.0;
	startLongitude.degrees = -73;
	startLongitude.minutes = -0.97759*60.0;

	// Final Latitude: 40.79869
	// Final Longitude: -73.95356
	nmea_position finalLatitude;
	nmea_position finalLongitude;
	finalLatitude.degrees = 40;
	finalLatitude.minutes = 0.79869*60.0;
	finalLongitude.degrees = -73;
	finalLongitude.minutes = -0.95356*60.0;

	float startLat = startLatitude.degrees + (startLatitude.minutes/100.0);
	float startLong = startLongitude.degrees + (startLongitude.minutes/100.0);
	float finalLat = finalLatitude.degrees + (finalLatitude.minutes/100.0);
	float finalLong = finalLongitude.degrees + (finalLongitude.minutes/100.0);
	ESP_LOGI(TAG, "startLat=%f finalLat=%f", startLat, finalLat);
	ESP_LOGI(TAG, "startLong=%f finalLong=%f", startLong, finalLong);
	float deltaLat = (finalLat - startLat)/STEPS;
	float deltaLong = (finalLong - startLong)/STEPS;
	ESP_LOGI(TAG, "deltaLong=%f deltalat=%f", deltaLong, deltaLat);

	float currentLat = startLat;
	float currentLong = startLong;
	nmea_position currentLatitude;
	nmea_position currentLongitude;
#if 0
	for (int i=0;i<STEPS;i++) {
		currentLat = currentLat + deltaLat;
		currentLong = currentLong + deltaLong;
		ESP_LOGI(TAG, "currentLong=%f currentlat=%f", currentLong, currentLat);
		currentLatitude.degrees = currentLat;
		currentLatitude.minutes = (currentLat - currentLatitude.degrees) * 100;
		currentLongitude.degrees = currentLong;
		currentLongitude.minutes = (currentLong - currentLongitude.degrees) * 100;
		ESP_LOGI(TAG, "currentLatitude=%d/%f", currentLatitude.degrees, currentLatitude.minutes);
		ESP_LOGI(TAG, "currentLongitude=%d/%f", currentLongitude.degrees, currentLongitude.minutes);
	}
#endif

	int counter = 0;
	while(1) {
		// Whether the browser has started or not
		// If the browser is not started, do not send the message.
		EventBits_t bits = xEventGroupGetBits(xEventWebSocket);
		ESP_LOGI(TAG, "bits=0x%x counter=%d", bits, counter);
		if (bits == 0x00) {
			vTaskDelay(100);
			continue;
		} else if (bits == 0x01) {
			currentLongitude.degrees = startLongitude.degrees;
			currentLongitude.minutes = startLongitude.minutes;
			currentLatitude.degrees = startLatitude.degrees;
			currentLatitude.minutes = startLatitude.minutes;
			currentLong = startLong;
			currentLat = startLat;
			counter = 0;
		} else if (bits == 0x03) {
			if (counter > STEPS) {
				vTaskDelay(100);
				continue;
			}
		}

		// Send current location to web client
		cJSON *request;
		request = cJSON_CreateObject();
		cJSON_AddStringToObject(request, "id", "nmea-request");
		cJSON_AddNumberToObject(request, "longitude_degrees", currentLongitude.degrees);
		cJSON_AddNumberToObject(request, "longitude_minutes", currentLongitude.minutes);
		cJSON_AddNumberToObject(request, "latitude_degrees", currentLatitude.degrees);
		cJSON_AddNumberToObject(request, "latitude_minutes", currentLatitude.minutes);
		cJSON_AddNumberToObject(request, "zoom_level", 15);
		cJSON_AddNumberToObject(request, "options", 0x02); // Disable zoom function
		char *nmea_string = cJSON_Print(request);
		ESP_LOGD(TAG, "nmea_string\n%s",nmea_string);
		size_t xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, nmea_string, strlen(nmea_string), NULL);
		if (xBytesSent != strlen(nmea_string)) {
			ESP_LOGE(TAG, "xMessageBufferSend fail");
		}
		cJSON_Delete(request);
		cJSON_free(nmea_string);

		// Calculate next localtion
		currentLat = currentLat + deltaLat;
		currentLong = currentLong + deltaLong;
		ESP_LOGI(TAG, "currentLong=%f currentlat=%f", currentLong, currentLat);
		currentLatitude.degrees = currentLat;
		currentLatitude.minutes = (currentLat - currentLatitude.degrees) * 100;
		currentLongitude.degrees = currentLong;
		currentLongitude.minutes = (currentLong - currentLongitude.degrees) * 100;
		ESP_LOGI(TAG, "currentLatitude=%d/%f", currentLatitude.degrees, currentLatitude.minutes);
		ESP_LOGI(TAG, "currentLongitude=%d/%f", currentLongitude.degrees, currentLongitude.minutes);
		counter++;
		vTaskDelay(200);
	}
	vTaskDelete(NULL);
}
