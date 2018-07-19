#include <stdlib.h>
#include <stdio.h>
#include "UID_bchainBTC.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

extern cache_buffer *current, *secondb;

char *load_tprv(char *privateKey, size_t size)
{
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%d) opening NVS handle!\n", err);
		privateKey = NULL;
        return privateKey;
    } else {
        printf("Done\n");

        // Read
        printf("Reading tprv from NVS ... ");
		size_t lenght = 0;
		err = nvs_get_str(my_handle, "privateKey", NULL, &lenght);
		err = nvs_get_str(my_handle, "privateKey", privateKey, &lenght);
        if(lenght>0)
		    printf("Unique tprv %s %d\n", privateKey, lenght);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                break;
            default :
                privateKey = NULL;
                break;
        }
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        nvs_close(my_handle);
    }
    return privateKey;
}

void store_tprv(char *privateKey)
{
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%d) opening NVS handle!\n", err);
		privateKey = NULL;
        return;
    } else {
        printf("Done\n");

        err = nvs_set_str(my_handle, "privateKey", privateKey);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        nvs_close(my_handle);
    }
    return;
}

void store_contracts(char * name, uint8_t *contracts, size_t size)//, int entries)
{
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%d) opening NVS handle!\n", err);
        return;
    } else {
        printf("Done\n");

        err = nvs_set_blob(my_handle, name, contracts, size);

        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        nvs_close(my_handle);
    }
    return;
}

void load_contracts(cache_buffer ** cache){
    *cache = current;
    (current->validCacheEntries) = 0;
    (current->validClientEntries) = 0; 

    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%d) opening NVS handle!\n", err);
        return;
    } else {
        printf("Done\n");

        // Read
        printf("Reading contracts from NVS ... ");
		size_t lenght = 0;
		err = nvs_get_blob(my_handle, "/contracts.bin", NULL, &lenght);
		err = nvs_get_blob(my_handle, "/contracts.bin", current, &lenght);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                break;
            default :
                break;
        }
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        nvs_close(my_handle);
    }
    return;
}
