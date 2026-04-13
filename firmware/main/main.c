/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "esp_camera.h"
// #include "esp_spiffs.h"

#include "http_server.h"
#include "camera.h"

void app_main(void)
{
    printf("woah look my code is running\n");
    fflush(stdout);

    vTaskDelay(pdMS_TO_TICKS(1000));

    http_server_init();
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    esp_err_t err = camera_init();

    if (err)
    {
        printf("Camera init failed - %d", err);
    }
    else
    {
        printf("Camera initialized");
    }

    printf("Initialization successful\n");
    fflush(stdout);
    
    vTaskDelay(pdMS_TO_TICKS(500));

    camera_fb_t *img = NULL;

    while (true)
    {
        static int i = 0;
        img = camera_get_img();
        http_server_update_img(img);

        i += 1;

        // printf("The program is running (%d)\n", i);
        // fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(200));

        camera_release_img(img);
    }

    // vTaskStartScheduler();
}
