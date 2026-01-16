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

static const char *TAG = "trip";

#include "websocket_server.h"

extern MessageBufferHandle_t xMessageBufferToClient;
extern EventGroupHandle_t xEventWebSocket;

#define STEPS 20

void trip_task(void* pvParameters)
{
	ESP_LOGW(TAG, "Trip Mode");
	// This mode traverses Central Park.
	// Starting position: 40.76631,-7397759
	// 40.76631 -> 0.76631*60=45.9786
	// -73.97759 -> -0.97759*60=-58.6554
	nmea_position startLatitude;
	nmea_position startLongitude;
	startLatitude.degrees = 40;
	startLatitude.minutes = 45.9786;
	startLongitude.degrees = -73;
	startLongitude.minutes = -58.6554;

	// End position: 40.79869, -73,95356
	nmea_position endLatitude;
	nmea_position endLongitude;
	endLatitude.degrees = 40;
	endLatitude.minutes = 47.9214;
	endLongitude.degrees = -73;
	endLongitude.minutes = -57.2136;

	float startLat = startLatitude.degrees + (startLatitude.minutes/100.0);
	float startLong = startLongitude.degrees + (startLongitude.minutes/100.0);
	float endLat = endLatitude.degrees + (endLatitude.minutes/100.0);
	float endLong = endLongitude.degrees + (endLongitude.minutes/100.0);
	ESP_LOGI(TAG, "startLat=%f endLat=%f", startLat, endLat);
	ESP_LOGI(TAG, "startLong=%f endLong=%f", startLong, endLong);
	float deltaLat = (endLat - startLat)/STEPS;
	float deltaLong = (endLong - startLong)/STEPS;
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

		// send current location to web client
		cJSON *request;
		request = cJSON_CreateObject();
		cJSON_AddStringToObject(request, "id", "nmea-request");
		cJSON_AddNumberToObject(request, "longitude_degrees", currentLongitude.degrees);
		cJSON_AddNumberToObject(request, "longitude_minutes", currentLongitude.minutes);
		cJSON_AddNumberToObject(request, "latitude_degrees", currentLatitude.degrees);
		cJSON_AddNumberToObject(request, "latitude_minutes", currentLatitude.minutes);
		cJSON_AddNumberToObject(request, "options", 0x02); // Disable zoom function
		char *my_json_string = cJSON_Print(request);
		ESP_LOGD(TAG, "my_json_string\n%s",my_json_string);
		size_t xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, my_json_string, strlen(my_json_string), NULL);
		if (xBytesSent != strlen(my_json_string)) {
			ESP_LOGE(TAG, "xMessageBufferSend fail");
		}
		cJSON_Delete(request);
		cJSON_free(my_json_string);

		// calculate next localtion
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
