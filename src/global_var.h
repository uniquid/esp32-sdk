#ifndef _GLOBAL__VAR_H_
#define _GLOBAL__VAR_H_

#include <fcntl.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include <freertos/queue.h>
#include <freertos/timers.h>

#include "UID_globals.h"
#include "UID_bchainBTC.h"

#include "esp_log.h"

extern bool apFlag;
extern QueueHandle_t provider_queue, user_queue;

extern char imprinting[];
extern char myname[];
extern uint16_t myname_size;
extern uint16_t imprinting_size;

typedef struct data {
    uint8_t type; // 0 from mqtt, 1 from ble
    char destination[BTC_ADDRESS_MAX_LENGHT];
    char * msg;
    size_t len; //length of msg without '\0' like strlen(msg)
} data_header;

#endif