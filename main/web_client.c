/*
	Example using WEB Socket.
	This example code is in the Public Domain (or CC0 licensed, at your option.)
	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "web_client";

#include "websocket_server.h"

extern MessageBufferHandle_t xMessageBufferToClient;
extern EventGroupHandle_t xEventWebSocket;

#define SOCKET_INIT_BIT BIT0
#define SOCKET_SEND_BIT BIT1

#if 0
// Timer callback
static void timer_cb(TimerHandle_t xTimer)
{
	TickType_t nowTick;
	nowTick = xTaskGetTickCount();
	ESP_LOGD(TAG, "timer is called, now=%"PRIu32, nowTick);

	cJSON *request;
	request = cJSON_CreateObject();
	cJSON_AddStringToObject(request, "id", "timer-request");
	char *my_json_string = cJSON_Print(request);
	ESP_LOGD(TAG, "my_json_string\n%s",my_json_string);
	size_t xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, my_json_string, strlen(my_json_string), NULL);
	if (xBytesSent != strlen(my_json_string)) {
		ESP_LOGE(TAG, "xMessageBufferSend fail");
	}
	cJSON_Delete(request);
	cJSON_free(my_json_string);
}
#endif

void client_task(void* pvParameters) {
#if 0
	// Create Timer (Trigger a measurement every second)
	TimerHandle_t timerHandle = xTimerCreate("MY Trigger", 500, pdTRUE, NULL, timer_cb);
	if (timerHandle != NULL) {
		if (xTimerStart(timerHandle, 0) != pdPASS) {
			ESP_LOGE(TAG, "Unable to start Timer");
			vTaskDelete(NULL);
		} else {
			ESP_LOGI(TAG, "Success to start Timer");
		}
	} else {
		ESP_LOGE(TAG, "Unable to create Timer");
		vTaskDelete(NULL);
	}
#endif

	char jsonBuffer[512];
	char DEL = 0x04;
	char outBuffer[128];

	while (1) {
		size_t readBytes = xMessageBufferReceive(xMessageBufferToClient, jsonBuffer, sizeof(jsonBuffer), portMAX_DELAY );
		ESP_LOGI(TAG, "readBytes=%d", readBytes);
		ESP_LOGI(TAG, "jsonBuffer=[%.*s]", readBytes, jsonBuffer);
		cJSON *root = cJSON_Parse(jsonBuffer);
		if (cJSON_GetObjectItem(root, "id")) {
			char *id = cJSON_GetObjectItem(root,"id")->valuestring;
			ESP_LOGI(TAG, "id=[%s]",id);

			if ( strcmp (id, "init") == 0) {
				sprintf(outBuffer,"HEAD%cWebsocket Template", DEL);
				ESP_LOGD(TAG, "outBuffer=[%s]", outBuffer);
				ws_server_send_text_all(outBuffer,strlen(outBuffer));

				// All clear xEventWebSocket
				xEventGroupClearBits(xEventWebSocket, SOCKET_INIT_BIT);
				xEventGroupClearBits(xEventWebSocket, SOCKET_SEND_BIT);
				EventBits_t bits = xEventGroupGetBits(xEventWebSocket);
				ESP_LOGI(TAG, "bits=0x%x", bits);

				// Set SOCKET_INIT_BIT to xEventWebSocket
				bits = bits | SOCKET_INIT_BIT;
				ESP_LOGI(TAG, "bits=0x%x", bits);
				xEventGroupSetBits(xEventWebSocket, bits);
			} // end if

			// for test
			if ( strcmp (id, "timer-request") == 0) {
				int lonDegrees = 139;
				float lonMinutes = 31.503430;
				int latDegrees = 35; 
				float latMinutes = 36.546690;
				sprintf(outBuffer,"DATA%c%d%c%f%c%d%c%f", DEL, latDegrees, DEL, latMinutes, DEL, lonDegrees, DEL, lonMinutes);
				ESP_LOGI(TAG, "outBuffer=[%s]", outBuffer);
				ws_server_send_text_all(outBuffer,strlen(outBuffer));

				ESP_LOGI(TAG, "esp_get_free_heap_size=%"PRIu32, esp_get_free_heap_size());
			} // timer-request

			if ( strcmp (id, "nmea-request") == 0) {
				int lonDegrees = cJSON_GetObjectItem(root,"longitude_degrees")->valueint;
				double lonMinutes = cJSON_GetObjectItem(root,"longitude_minutes")->valuedouble;
				int latDegrees= cJSON_GetObjectItem(root,"latitude_degrees")->valueint;
				double latMinutes= cJSON_GetObjectItem(root,"latitude_minutes")->valuedouble;
				int zoomLevel = cJSON_GetObjectItem(root,"zoom_level")->valueint;
				int zoomOptions = cJSON_GetObjectItem(root,"options")->valueint;
				ESP_LOGI(TAG, "lonDegrees=%d lonMinutes=%f", lonDegrees, lonMinutes);
				ESP_LOGI(TAG, "latDegrees%d latMinutes%f", latDegrees, latMinutes);
				ESP_LOGI(TAG, "zoomLevel=%d zoomOptions=0x%x", zoomLevel, zoomOptions);
				sprintf(outBuffer,"DATA%c%d%c%f%c%d%c%f%c%d%c%d",
					DEL, latDegrees, DEL, latMinutes, DEL, lonDegrees, DEL, lonMinutes, DEL, zoomLevel, DEL, zoomOptions);
				ESP_LOGI(TAG, "outBuffer=[%s]", outBuffer);
				ws_server_send_text_all(outBuffer,strlen(outBuffer));

				ESP_LOGI(TAG, "esp_get_free_heap_size=%"PRIu32, esp_get_free_heap_size());

				// Set SOCKET_SEND_BIT to xEventWebSocket
				EventBits_t bits = xEventGroupGetBits(xEventWebSocket);
				bits = bits | SOCKET_SEND_BIT;
				ESP_LOGI(TAG, "bits=0x%x", bits);
				xEventGroupSetBits(xEventWebSocket, bits);
			} // nmea-request

			if ( strcmp (id, "zoomlevel-request") == 0) {
				int zoomLevel = cJSON_GetObjectItem(root,"zoom_level")->valueint;
				ESP_LOGI(TAG, "zoomLevel=%d", zoomLevel);
				sprintf(outBuffer,"ZOOMLEVEL%c%d", DEL, zoomLevel);
				ESP_LOGI(TAG, "outBuffer=[%s]", outBuffer);
				ws_server_send_text_all(outBuffer,strlen(outBuffer));
			} // zoomlevel-request

			if ( strcmp (id, "zoomcontrol-request") == 0) {
				int zoomControl = cJSON_GetObjectItem(root,"zoom_control")->valueint;
				ESP_LOGI(TAG, "zoomControl=%d", zoomControl);
				sprintf(outBuffer,"ZOOMCONTROL%c%d", DEL, zoomControl);
				ESP_LOGI(TAG, "outBuffer=[%s]", outBuffer);
				ws_server_send_text_all(outBuffer,strlen(outBuffer));
			} // zoomcontrol-request

			if ( strcmp (id, "fullscreen-request") == 0) {
				int fullscreen = cJSON_GetObjectItem(root,"fullscreen")->valueint;
				ESP_LOGI(TAG, "fullscreen=%d", fullscreen);
				sprintf(outBuffer,"FULLSCREEN%c%d", DEL, fullscreen);
				ESP_LOGI(TAG, "outBuffer=[%s]", outBuffer);
				ws_server_send_text_all(outBuffer,strlen(outBuffer));
			} // fullscreen-request

			if ( strcmp (id, "message-request") == 0) {
				char *position = cJSON_GetObjectItem(root,"position")->valuestring;
				int timeout = cJSON_GetObjectItem(root,"timeout")->valueint;
				char *message = cJSON_GetObjectItem(root,"message")->valuestring;
				ESP_LOGI(TAG, "position=[%s] timeout=%d message=[%s]", position, timeout, message);
				sprintf(outBuffer,"MESSAGEBOX%c%s%c%d%c%s", DEL, position, DEL, timeout, DEL, message);
				ESP_LOGI(TAG, "outBuffer=[%s]", outBuffer);
				ws_server_send_text_all(outBuffer,strlen(outBuffer));
			} // message-request

		} // end if

		// Delete a cJSON structure
		cJSON_Delete(root);

	} // end while

	// Never reach here
	vTaskDelete(NULL);
}
