#include "global_var.h"

bool apFlag = false;
QueueHandle_t provider_queue;

uint16_t imprinting_size = 1024;
char imprinting[1024] = {0};
char myname[UID_NAME_LENGHT] = {0};
