#include "http_server.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_check.h"
#include <time.h>
#include <sys/time.h>
#if !CONFIG_IDF_TARGET_LINUX
#include <esp_wifi.h>
#include <esp_system.h>
#include "nvs_flash.h"
#endif  // !CONFIG_IDF_TARGET_LINUX

#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define WIFI_SSID "mywifissid"
*/
#define ESP_WIFI_SSID      "Haptic Glove"
#define ESP_WIFI_PASS      "password"
#define ESP_WIFI_CHANNEL   6 // 1 or 6
#define MAX_STA_CONN       5

#define STREAM_CONTENT_TYPE "multipart/x-mixed-replace;boundary=frame"
#define STREAM_BOUNDARY "\r\n--frame\r\n"
#define STREAM_PART "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n"

static camera_fb_t *camera_img = NULL;
static bool img_updated = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;

        printf("station "MACSTR" join, AID=%d\n", MAC2STR(event->mac), event->aid);
        fflush(stdout);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;

        printf("station "MACSTR" leave, AID=%d, reason=%d\n", MAC2STR(event->mac), event->aid, event->reason);
        fflush(stdout);
    }
}

void http_server_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("wifi_init_softap finished. SSID:%s password:%s channel:%d\n", ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_WIFI_CHANNEL);
    fflush(stdout);
}

static esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        printf("=========== RECEIVED DATA ==========");
        printf("%.*s", ret, buf);
        printf("====================================");
        fflush(stdout);
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t echo_uri = {
    .uri       = "/echo",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = NULL
};

static esp_err_t index_handler(httpd_req_t *req)
{
    const char* index_html =
        "<html>"
        "<body>"
        "<h1>ESP32 Video Stream</h1>"
        "<img src=\"/jpeg_stream\">"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
};

static esp_err_t jpeg_stream_handler(httpd_req_t *req)
{
    esp_err_t err = 0;
    httpd_resp_set_type(req, STREAM_CONTENT_TYPE);

    char part_buf[64];

    while (1) {

        // Send boundary
        httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));

        // Send header for this frame
        int hlen = snprintf(part_buf, 64, STREAM_PART, camera_img->len);
        httpd_resp_send_chunk(req, part_buf, hlen);

        // Send image data
        httpd_resp_send_chunk(req, (const char*)camera_img->buf, camera_img->len);

        while (!img_updated)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        img_updated = false;
    }

    return ESP_OK;
}

httpd_uri_t jpeg_stream_uri = {
    .uri = "/jpeg_stream",
    .method = HTTP_GET,
    .handler = jpeg_stream_handler
};

esp_err_t http_server_update_img(camera_fb_t *img)
{
    camera_img = img;
    img_updated = true;
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    printf("Starting server on port: '%d'", config.server_port);
    fflush(stdout);

    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        printf("Registering URI handlers");
        fflush(stdout);
        httpd_register_uri_handler(server, &echo_uri);
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &jpeg_stream_uri);
        return server;
    }

    printf("Error starting server!");
    fflush(stdout);
    return NULL;
}

esp_err_t http_server_init(void)
{
    nvs_flash_init();

    http_server_init_softap();

    start_webserver();

    return ESP_OK;
}

esp_err_t http_server_start(void)
{
    return ESP_OK;
}

esp_err_t http_server_stop(void)
{
    return ESP_OK;
}