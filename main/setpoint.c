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

#include "esp_vfs.h"
#include "esp_spiffs.h"

static const char *TAG = "setpoint";

#include "websocket_server.h"

extern MessageBufferHandle_t xMessageBufferToClient;
extern EventGroupHandle_t xEventWebSocket;

typedef struct {
	float latitude;
	float longitude;
	char text[64];
	int zoomlevel;
} SETPOINT_t;

static void listSPIFFS(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

esp_err_t mountSPIFFS(char * path, char * label, int max_files) {
	esp_vfs_spiffs_conf_t conf = {
		.base_path = path,
		.partition_label = label,
		.max_files = max_files,
		.format_if_mount_failed = true
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret ==ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret== ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return ret;
	}

#if 0
	ESP_LOGI(TAG, "Performing SPIFFS_check().");
	ret = esp_spiffs_check(conf.partition_label);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
		return ret;
	} else {
			ESP_LOGI(TAG, "SPIFFS_check() successful");
	}
#endif

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(conf.partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Mount %s to %s success", path, label);
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}

	return ret;
}

static int parseLine(char *line, int size1, int size2, char arr[size1][size2])
{
	ESP_LOGD(TAG, "line=[%s]", line);
	int dst = 0;
	int pos = 0;
	int llen = strlen(line);
	bool inq = false;

	for(int src=0;src<llen;src++) {
		char c = line[src];
		ESP_LOGD(TAG, "src=%d c=%c", src, c);
		if (c == ',') {
			if (inq) {
				if (pos == (size2-1)) continue;
				arr[dst][pos++] = line[src];
				arr[dst][pos] = 0;
			} else {
				ESP_LOGD(TAG, "arr[%d]=[%s]",dst,arr[dst]);
				dst++;
				if (dst == size1) break;
				pos = 0;
			}

		} else if (c == ';') {
			if (inq) {
				if (pos == (size2-1)) continue;
				arr[dst][pos++] = line[src];
				arr[dst][pos] = 0;
			} else {
				ESP_LOGD(TAG, "arr[%d]=[%s]",dst,arr[dst]);
				dst++;
				break;
			}

		} else if (c == '"') {
			inq = !inq;

		} else if (c == '\'') {
			inq = !inq;

		} else {
			if (pos == (size2-1)) continue;
			arr[dst][pos++] = line[src];
			arr[dst][pos] = 0;
		}
	}

	return dst;
}

static int readDefineFile(char * path, SETPOINT_t *setpoint, size_t maxLine) {
	int readLine = 0;
	ESP_LOGI(pcTaskGetName(NULL), "Reading file[%s]",path);
	FILE* fp = fopen(path, "r");
	if (fp == NULL) {
		ESP_LOGE(pcTaskGetName(NULL), "Failed to open define file for reading");
		ESP_LOGE(pcTaskGetName(NULL), "Please make setpoint.def");
		return 0;
	}
	char line[64];
	char result[10][32];
	while (1){
		if ( fgets(line, sizeof(line) ,fp) == 0 ) break;
		// strip newline
		char* pos = strchr(line, '\n');
		if (pos) {
			*pos = '\0';
		}
		ESP_LOGI(pcTaskGetName(NULL), "line=[%s]", line);
		if (strlen(line) == 0) continue;
		if (line[0] == '#') continue;

		memset(result, 0, sizeof(result));
		int ret = parseLine(line, 10, 32, result);
		ESP_LOGI(TAG, "parseLine=%d", ret);
		for(int i=0;i<ret;i++) ESP_LOGI(TAG, "result[%d]=[%s]", i, &result[i][0]);
		setpoint[readLine].latitude = strtof(&result[0][0], NULL);
		setpoint[readLine].longitude = strtof(&result[1][0], NULL);
		strcpy(setpoint[readLine].text, &result[2][0]);
		setpoint[readLine].zoomlevel = 0;
		if (ret == 4) setpoint[readLine].zoomlevel = strtol(&result[3][0], NULL, 10);
		readLine++;
		if (readLine == maxLine) break;
	}
	fclose(fp);
	return readLine;
}

#define MAX_CONFIG 64

void setpoint_task(void* pvParameters)
{
	ESP_LOGW(TAG, "Set Point Mode");

	// Initialize SPIFFS
	ESP_ERROR_CHECK(mountSPIFFS("/setpoint", "storage", 1));
	listSPIFFS("/setpoint/");

	// Allocate memory
	SETPOINT_t *setpoint = malloc(sizeof(SETPOINT_t) * MAX_CONFIG);
	if (setpoint == NULL) {
		ESP_LOGE(TAG, "malloc fail");
		vTaskDelete(NULL);
	}

	// Read setpoint information
	int readLine = readDefineFile("/setpoint/setpoint.def", setpoint, MAX_CONFIG);
	ESP_LOGI(pcTaskGetName(NULL), "readLine=%d",readLine);
	if (readLine == 0) {
		while(1) { vTaskDelay(1); }
	}

	for(int i=0;i<readLine;i++) {
		ESP_LOGI(pcTaskGetName(NULL), "setpoint[%d].latitude=[%f]",i, setpoint[i].latitude);
		ESP_LOGI(pcTaskGetName(NULL), "setpoint[%d].longitude=[%f]",i, setpoint[i].longitude);
		ESP_LOGI(pcTaskGetName(NULL), "setpoint[%d].text=[%s]",i, setpoint[i].text);
		ESP_LOGI(pcTaskGetName(NULL), "setpoint[%d].zoomlevel=[%d]",i, setpoint[i].zoomlevel);
	}

	int maxZoomLevel = 8;
	if (setpoint[0].zoomlevel != 0) maxZoomLevel = setpoint[0].zoomlevel;
	ESP_LOGI(TAG, "maxZoomLevel=%d", maxZoomLevel);

	nmea_position currentLatitude;
	nmea_position currentLongitude;
	int pointCounter = 0;
	int zoomLevel = 0;
	bool messagebox = false;
	int options =0x02; // Disable zoom function

	while(1) {
		// Whether the browser has started or not
		// If the browser is not started, do not send the message.
		EventBits_t bits = xEventGroupGetBits(xEventWebSocket);
		ESP_LOGI(TAG, "bits=0x%x zoomLevel=%d pointCounter=%d", bits, zoomLevel, pointCounter);
		if (bits == 0x00) {
			vTaskDelay(100);
			continue;
		} else if (bits == 0x01) {
			pointCounter = 0;
			zoomLevel = 0;
			messagebox = false;
			options = 0x02; // Disable zoom function
		} else if (bits == 0x03) {
			if (pointCounter >= readLine) {
				vTaskDelay(100);
				continue;
			}
		}

		// Set current location
		currentLatitude.degrees = setpoint[pointCounter].latitude;
		currentLatitude.minutes = (setpoint[pointCounter].latitude - currentLatitude.degrees) * 60.0;
		currentLongitude.degrees = setpoint[pointCounter].longitude;
		currentLongitude.minutes = (setpoint[pointCounter].longitude - currentLongitude.degrees) * 60.0;
		ESP_LOGI(TAG, "currentLatitude=%d %f", currentLatitude.degrees, currentLatitude.minutes);
		ESP_LOGI(TAG, "currentLongitude=%d %f", currentLongitude.degrees, currentLongitude.minutes);

		// Send current location to web client
		cJSON *request;
		request = cJSON_CreateObject();
		cJSON_AddStringToObject(request, "id", "nmea-request");
		cJSON_AddNumberToObject(request, "longitude_degrees", currentLongitude.degrees);
		cJSON_AddNumberToObject(request, "longitude_minutes", currentLongitude.minutes);
		cJSON_AddNumberToObject(request, "latitude_degrees", currentLatitude.degrees);
		cJSON_AddNumberToObject(request, "latitude_minutes", currentLatitude.minutes);
		//cJSON_AddNumberToObject(request, "zoom_level", 8);
		cJSON_AddNumberToObject(request, "zoom_level", zoomLevel);
		cJSON_AddNumberToObject(request, "options", options); // Disable zoom function
		char *nmea_string = cJSON_Print(request);
		ESP_LOGD(TAG, "nmea_string\n%s",nmea_string);
		size_t xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, nmea_string, strlen(nmea_string), NULL);
		if (xBytesSent != strlen(nmea_string)) {
			ESP_LOGE(TAG, "xMessageBufferSend fail");
		}
		cJSON_Delete(request);
		cJSON_free(nmea_string);

		// Send message to web client
		if (messagebox && strlen(setpoint[pointCounter].text)) {
			request = cJSON_CreateObject();
			cJSON_AddStringToObject(request, "id", "message-request");
			cJSON_AddStringToObject(request, "position", "bottomright");
			cJSON_AddNumberToObject(request, "timeout", 3000);
			cJSON_AddStringToObject(request, "message", setpoint[pointCounter].text);
			char *message_string = cJSON_Print(request);
			ESP_LOGD(TAG, "message_string\n%s",message_string);
			xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, message_string, strlen(message_string), NULL);
			if (xBytesSent != strlen(message_string)) {
				ESP_LOGE(TAG, "xMessageBufferSend fail");
			}
			cJSON_Delete(request);
			cJSON_free(message_string);
		}

		// Set next map zoom size and localtion
		if (zoomLevel < maxZoomLevel) {
			zoomLevel++;
			if (zoomLevel == maxZoomLevel) {
				options = 0x03; // Set center of map & Disable zoom function
				messagebox = true;
			}
			vTaskDelay(100);
		} else {
			pointCounter++;
			vTaskDelay(300);
		}
	}

	// Never reach here
	vTaskDelete(NULL);
}
