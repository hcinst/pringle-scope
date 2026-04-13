#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <esp_err.h>
#include "esp_camera.h"

esp_err_t http_server_init(void);
esp_err_t http_server_start(void);
esp_err_t http_server_stop(void);
esp_err_t http_server_update_img(camera_fb_t *img);

#endif // HTTP_SERVER_H