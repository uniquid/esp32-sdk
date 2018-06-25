#include "nvs_flash.h"
#include "esp_err.h"
#include "global_var.h"

extern void main(void*);

void reader(void *arg)
{
    char c;
    int i = 0;
    while(1){
        c = getchar();
        if(c=='e'){
            printf("It's time to erase!\n");
            nvs_flash_erase();
        }
        else if(c=='r'){
            printf("It's time to restart!\n");
            esp_restart();
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        i++;
        if(i>10)
            vTaskDelete( NULL ); 
    }
}

void setup()
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

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
    xTaskCreate(&main, "main_task", 8*2048, NULL, 1, NULL);
    xTaskCreate(&reader, "reader", 1024, NULL, 1, NULL);
}