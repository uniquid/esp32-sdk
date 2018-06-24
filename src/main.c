#include "nvs_flash.h"
#include "esp_err.h"
#include "global_var.h"

extern void main(void*);

void setup()
{
    // Initialize NVS
    ESP_ERROR_CHECK( nvs_flash_init() );

    provider_queue = xQueueCreate( 5, sizeof( data_in_service ) );

    printf("UNIQUID @ESP32-SDK\n");
    printf("******************************************************************************************\r\n");
    printf("Genereting UNIQUID node\n");
    printf("******************************************************************************************\r\n");

    start_wifi();
    start_mqtt();
}


void app_main()
{
    setup();
    xTaskCreate(&main, "main_task", 4*2048, NULL, 1, NULL);
}