#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "camera.h"

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 10000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_VGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 20, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

esp_err_t camera_init(void)
{
    return esp_camera_init(&camera_config);
}

static bool camera_enabled = false;

bool camera_is_enabled(void)
{
    return camera_enabled;
}

void camera_enable(void)
{
    camera_enabled = true;
}

void camera_disable(void)
{
    camera_enabled = false;
}

camera_fb_t *camera_get_img(void)
{
    camera_fb_t *fb = esp_camera_fb_get();

    /*
    printf("Image captured: \n");
    
    switch (fb->format)
    {
    case PIXFORMAT_RGB565:
        printf("Format:     RGB565\n");
        break;
    case PIXFORMAT_YUV422:
        printf("Format:     YUV422\n");
        break;
    case PIXFORMAT_YUV420:
        printf("Format:     YUV420\n");
        break;
    case PIXFORMAT_GRAYSCALE:
        printf("Format:     GRAYSCALE\n");
        break;
    case PIXFORMAT_JPEG:
        printf("Format:     JPEG\n");
        break;
    case PIXFORMAT_RGB888:
        printf("Format:     RGB888\n");
        break;
    case PIXFORMAT_RAW:
        printf("Format:     RAW\n");
        break;
    case PIXFORMAT_RGB444:
        printf("Format:     RGB444\n");
        break;
    case PIXFORMAT_RGB555:
        printf("Format:     RGB555\n");
        break;
    default:
        printf("Format:     Unknown\n");
        break;
    }

    fflush(stdout);

    printf("Resolution: %ux%u\n", fb->width, fb->height);
    printf("Size:       %u bytes\n", fb->len);

    fflush(stdout);
    */

    return fb;
}

void camera_release_img(camera_fb_t *img)
{
    esp_camera_fb_return(img);
}