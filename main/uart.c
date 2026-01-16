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
#include "driver/uart.h"
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

static const char *TAG = "uart";

#include "websocket_server.h"

extern MessageBufferHandle_t xMessageBufferToClient;
extern EventGroupHandle_t xEventWebSocket;

#define UART_RX_BUF_SIZE (1024)

static char s_buf[UART_RX_BUF_SIZE + 1];
static size_t s_total_bytes;
static char *s_last_buf_end;

void init_interface(void)
{
	uart_config_t uart_config = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
		.source_clk = UART_SCLK_DEFAULT,
#endif
	};
	ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1,
		UART_PIN_NO_CHANGE, CONFIG_UART_RX_GPIO,
		UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, UART_RX_BUF_SIZE * 2, 0, 0, NULL, 0));
}

void nmea_read_line(char **out_line_buf, size_t *out_line_len, int timeout_ms)
{
	*out_line_buf = NULL;
	*out_line_len = 0;

	if (s_last_buf_end != NULL) {
		/* Data left at the end of the buffer after the last call;
		 * copy it to the beginning.
		 */
		size_t len_remaining = s_total_bytes - (s_last_buf_end - s_buf);
		memmove(s_buf, s_last_buf_end, len_remaining);
		s_last_buf_end = NULL;
		s_total_bytes = len_remaining;
	}

	/* Read data from the UART */
	int read_bytes = uart_read_bytes(UART_NUM_1,
		(uint8_t *) s_buf + s_total_bytes,
		UART_RX_BUF_SIZE - s_total_bytes, pdMS_TO_TICKS(timeout_ms));
	if (read_bytes <= 0) {
		return;
	}
	s_total_bytes += read_bytes;

	/* find start (a dollar sign) */
	char *start = memchr(s_buf, '$', s_total_bytes);
	if (start == NULL) {
		s_total_bytes = 0;
		return;
	}

	/* find end of line */
	char *end = memchr(start, '\r', s_total_bytes - (start - s_buf));
	if (end == NULL || *(++end) != '\n') {
		return;
	}
	end++;

	end[-2] = NMEA_END_CHAR_1;
	end[-1] = NMEA_END_CHAR_2;

	*out_line_buf = start;
	*out_line_len = end - start;
	if (end < s_buf + s_total_bytes) {
		/* some data left at the end of the buffer, record its position until the next call */
		s_last_buf_end = end;
	} else {
		s_total_bytes = 0;
	}
}

static void read_and_parse_nmea()
{
	while (1) {
		char fmt_buf[32];
		nmea_s *data;

		char *start;
		size_t length;
		nmea_read_line(&start, &length, 100 /* ms */);
		if (length == 0) {
			continue;
		}

		printf("%.*s", length, start);
		/* handle data */
		data = nmea_parse(start, length, 0);
		if (data == NULL) {
			ESP_LOGE(TAG, "Failed to parse the sentence!");
			ESP_LOGE(TAG, "Type: %.5s (%d)", start + 1, nmea_get_type(start));
		} else {
			if (data->errors != 0) {
				ESP_LOGE(TAG, "The sentence struct contains parse errors!");
			}

			if (NMEA_GPGGA == data->type) {
#if 0
				printf("GPGGA sentence\n");
				nmea_gpgga_s *gpgga = (nmea_gpgga_s *) data;
				printf("Number of satellites: %d\n", gpgga->n_satellites);
				printf("Altitude: %f %c\n", gpgga->altitude,
					gpgga->altitude_unit);
#endif
			}

			if (NMEA_GPGLL == data->type) {
#if 0
				printf("GPGLL sentence\n");
				nmea_gpgll_s *pos = (nmea_gpgll_s *) data;
				printf("Longitude:\n");
				printf("  Degrees: %d\n", pos->longitude.degrees);
				printf("  Minutes: %f\n", pos->longitude.minutes);
				printf("  Cardinal: %c\n", (char) pos->longitude.cardinal);
				printf("Latitude:\n");
				printf("  Degrees: %d\n", pos->latitude.degrees);
				printf("  Minutes: %f\n", pos->latitude.minutes);
				printf("  Cardinal: %c\n", (char) pos->latitude.cardinal);
				strftime(fmt_buf, sizeof(fmt_buf), "%H:%M:%S", &pos->time);
				printf("Time: %s\n", fmt_buf);
#endif
			}

			if (NMEA_GPRMC == data->type) {
				printf("GPRMC sentence\n");
				nmea_gprmc_s *pos = (nmea_gprmc_s *) data;
				printf("Longitude:\n");
				printf("  Degrees: %d\n", pos->longitude.degrees);
				printf("  Minutes: %f\n", pos->longitude.minutes);
				printf("  Cardinal: %c\n", (char) pos->longitude.cardinal);
				printf("Latitude:\n");
				printf("  Degrees: %d\n", pos->latitude.degrees);
				printf("  Minutes: %f\n", pos->latitude.minutes);
				printf("  Cardinal: %c\n", (char) pos->latitude.cardinal);
#if 0
				strftime(fmt_buf, sizeof(fmt_buf), "%d %b %T %Y", &pos->date_time);
				printf("Date & Time: %s\n", fmt_buf);
				printf("Speed, in Knots: %f\n", pos->gndspd_knots);
				printf("Track, in degrees: %f\n", pos->track_deg);
				printf("Magnetic Variation:\n");
				printf("  Degrees: %f\n", pos->magvar_deg);
				printf("  Cardinal: %c\n", (char) pos->magvar_cardinal);
				double adjusted_course = pos->track_deg;
				if (NMEA_CARDINAL_DIR_EAST == pos->magvar_cardinal) {
					adjusted_course -= pos->magvar_deg;
				} else if (NMEA_CARDINAL_DIR_WEST == pos->magvar_cardinal) {
					adjusted_course += pos->magvar_deg;
				} else {
					printf("Invalid Magnetic Variation Direction!\n");
				}

				printf("Adjusted Track (heading): %f\n", adjusted_course);
#endif

				if (pos->longitude.degrees == 0) continue;
				if (pos->latitude.degrees == 0) continue;

				// Whether the browser has started or not
				// If the browser is not started, do not send the message.
				EventBits_t bits = xEventGroupGetBits(xEventWebSocket);
				ESP_LOGI(TAG, "bits=0x%x", bits);
				if (bits != 0x01) continue;

				// Send current location to web client
				cJSON *request;
				request = cJSON_CreateObject();
				cJSON_AddStringToObject(request, "id", "nmea-request");
				cJSON_AddNumberToObject(request, "longitude_degrees", pos->longitude.degrees);
				cJSON_AddNumberToObject(request, "longitude_minutes", pos->longitude.minutes);
				cJSON_AddNumberToObject(request, "latitude_degrees", pos->latitude.degrees);
				cJSON_AddNumberToObject(request, "latitude_minutes", pos->latitude.minutes);
				cJSON_AddNumberToObject(request, "zoom_level", 15);
				cJSON_AddNumberToObject(request, "options", 1); // Use Fullscreen Control
				char *nmea_string = cJSON_Print(request);
				ESP_LOGD(TAG, "nmea_string\n%s",nmea_string);
				size_t xBytesSent = xMessageBufferSendFromISR(xMessageBufferToClient, nmea_string, strlen(nmea_string), NULL);
				if (xBytesSent != strlen(nmea_string)) {
					ESP_LOGE(TAG, "xMessageBufferSend fail");
				}
				cJSON_Delete(request);
				cJSON_free(nmea_string);

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

			if (NMEA_GPGSA == data->type) {
#if 0
				printf("GPGSA Sentence:\n");
				nmea_gpgsa_s *gpgsa = (nmea_gpgsa_s *) data;
				printf("  Mode: %c\n", gpgsa->mode);
				printf("  Fix:	%d\n", gpgsa->fixtype);
				printf("  PDOP: %.2lf\n", gpgsa->pdop);
				printf("  HDOP: %.2lf\n", gpgsa->hdop);
				printf("  VDOP: %.2lf\n", gpgsa->vdop);
#endif
			}

			if (NMEA_GPGSV == data->type) {
#if 0
				printf("GPGSV Sentence:\n");
				nmea_gpgsv_s *gpgsv = (nmea_gpgsv_s *) data;
				printf("  Num: %d\n", gpgsv->sentences);
				printf("  ID:  %d\n", gpgsv->sentence_number);
				printf("  SV:  %d\n", gpgsv->satellites);
				printf("  #1:  %d %d %d %d\n", gpgsv->sat[0].prn, gpgsv->sat[0].elevation, gpgsv->sat[0].azimuth, gpgsv->sat[0].snr);
				printf("  #2:  %d %d %d %d\n", gpgsv->sat[1].prn, gpgsv->sat[1].elevation, gpgsv->sat[1].azimuth, gpgsv->sat[1].snr);
				printf("  #3:  %d %d %d %d\n", gpgsv->sat[2].prn, gpgsv->sat[2].elevation, gpgsv->sat[2].azimuth, gpgsv->sat[2].snr);
				printf("  #4:  %d %d %d %d\n", gpgsv->sat[3].prn, gpgsv->sat[3].elevation, gpgsv->sat[3].azimuth, gpgsv->sat[3].snr);
#endif
			}

			if (NMEA_GPTXT == data->type) {
#if 0
				printf("GPTXT Sentence:\n");
				nmea_gptxt_s *gptxt = (nmea_gptxt_s *) data;
				printf("  ID: %d %d %d\n", gptxt->id_00, gptxt->id_01, gptxt->id_02);
				printf("  %s\n", gptxt->text);
#endif
			}

			if (NMEA_GPVTG == data->type) {
#if 0
				printf("GPVTG Sentence:\n");
				nmea_gpvtg_s *gpvtg = (nmea_gpvtg_s *) data;
				printf("  Track [deg]:	 %.2lf\n", gpvtg->track_deg);
				printf("  Speed [kmph]:  %.2lf\n", gpvtg->gndspd_kmph);
				printf("  Speed [knots]: %.2lf\n", gpvtg->gndspd_knots);
#endif
			}

			nmea_free(data);
		}
	}
}

void uart_task(void* pvParameters)
{
	init_interface();
	read_and_parse_nmea();
}
