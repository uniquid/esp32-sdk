#include <unistd.h>
#include <errno.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_wifi_types.h"
#include "esp_wifi.h"

#define WIFI_TIMER_SEC 5
TimerHandle_t wifiReconnectTimer;

static void wifi_reconnect(void *arg)
{
    ESP_ERROR_CHECK(esp_wifi_connect());
    xTimerStop(wifiReconnectTimer,0);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    printf("[WiFi-event] event: %d\n", event->event_id);

    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START: //2
            printf("WIFI: start\n");
            ESP_ERROR_CHECK(esp_wifi_connect());
            break;
        case SYSTEM_EVENT_STA_STOP: //3
            printf("WIFI: stop\n");
            break;
        case SYSTEM_EVENT_STA_CONNECTED: //4
            printf("WIFI: connected\n");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED: //5
            printf("WIFI: lost connection\n");
            //apFlag =  false;
            xTimerStart(wifiReconnectTimer,0);
            break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE: //6
            break;
        case SYSTEM_EVENT_STA_GOT_IP: //7
            printf("WIFI: got ip:%s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            printf("WIFI: got gateway ip:%s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.gw));
            //apFlag =  true;
            break;
        default:
            break;
    }
    return ESP_OK;
}

void start_wifi()
{
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0, wifi_reconnect);
    char ssid[100], pass[100];
    wifi_config_t wifi_config = {
        .sta = {MY_SSID, MY_PASSWORD}
    };
    printf("WIFI: config parameters from DEFINE -> ssid:%s, password:%s\n", MY_SSID, MY_PASSWORD);
 
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    printf("Wait for WiFi...\n");   
    vTaskDelay(1000);
    printf("Going on...\n");
}
