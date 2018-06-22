#include "nvs_flash.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

void setup()
{
    // Initialize NVS
    ESP_ERROR_CHECK( nvs_flash_init() );

    printf("UNIQUID @ESP32-SDK\n");
    printf("******************************************************************************************\r\n");
    printf("Genereting UNIQUID node\n");
    printf("******************************************************************************************\r\n");

    start_wifi();
    start_mqtt();
    main();
}


void app_main()
{
    setup();
    while(1){
        // never reached
        printf("## ERROR ##\nthis statement should never be reached!!\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}