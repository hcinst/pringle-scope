idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "../components/esp32-camera/driver/include" 
                                 "../components/camera/include"
                                 "../components/wifi/include"
                    REQUIRES nvs_flash spi_flash wifi)