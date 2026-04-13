#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"

esp_err_t camera_init(void);
bool camera_is_enabled(void);
void camera_enable(void);
void camera_disable(void);
camera_fb_t *camera_get_img(void);
void camera_release_img(camera_fb_t *img);

#endif