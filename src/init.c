#include "UID_bchainBTC.h"
#include "UID_fillCache.h"
#include "UID_identity.h"
#include "UID_utils.h"

#include "global_var.h"

#include <freertos/task.h>
#include <freertos/ringbuf.h>

#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

extern cache_buffer *current, *secondb;

void store_contracts(char* name, uint8_t * contracts, size_t size, int entries);
void load_contracts(cache_buffer ** cache);
void service_provider(data_in_service* arg);

void printCache(cache_buffer *cache)
{
    int i;
    char buf[161];
	printf("[\n");
    for (i = 0; i<cache->validCacheEntries;i++) {
        printf("[ %s %s %s ]\n",
            cache->contractsCache[i].serviceProviderAddress,
            cache->contractsCache[i].serviceUserAddress,
            tohex((uint8_t *)&(cache->contractsCache[i].profile), 80, buf));
    }
	printf("][\n");
    for (i = 0; i<cache->validClientEntries;i++) {
        printf("[ %s %s <%s> ]\n",
            cache->clientCache[i].serviceProviderAddress,
            cache->clientCache[i].serviceUserAddress,
            cache->clientCache[i].serviceProviderName);
    }
    printf("]\n");
}

// Update Cache Thread
// gets contracts from the BlockChain and updates the local cache
void updater(void *arg)
{
	cache_buffer *cache;
	bool apFlagTemp = false;
	int ret;
	while(1){
		apFlagTemp=apFlag;
		if(apFlagTemp){
			printf("UID_getContracts\n");
			ret = UID_getContracts(&cache);
			if ( UID_CONTRACTS_OK == ret) {
				bool cacheWrite = false;
				if (current->validCacheEntries != secondb->validCacheEntries){
					cacheWrite = true;
				}
				else {
					if (memcmp(current->contractsCache, secondb->contractsCache, sizeof(UID_SecurityProfile)*(current->validCacheEntries))){
						cacheWrite = true;
					}
				}
				if (cacheWrite){
					store_contracts("/ccache.bin", (uint8_t *)(cache->contractsCache), sizeof(UID_SecurityProfile)*(cache->validCacheEntries), cache->validCacheEntries);
					printf("Do cache write\n");
				}
				else{
					printf("Skipping cache write\n");
				}
			}
			else{
				// timeout o errori
				printf("INIT: UID_getContracts() return %d", ret);
			}
			// just print the cache contents
			printCache(cache);
			vTaskDelay(45000 / portTICK_PERIOD_MS);
		} else {
			vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}
}

void main(void *arg)
{
	uint16_t quotient = 0, remainder = 0, i = 0;
	char slen[5] = {0};
	data_in_service main_in;
	cache_buffer *cache;
	// set up the URL to insight-api appliance
	UID_pApplianceURL = "http://explorer.uniquid.co:3001/insight-api";
	// set up the URL to the registry appliance
	UID_pRegistryURL = REGISTRY_URL;
	UID_getLocalIdentity(NULL);
	load_contracts(&cache);
	printCache(cache);

	printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%d) opening NVS handle!\n", err);
		esp_restart();
    } else {
        printf("Done\n");

        // Read
        printf("Reading restart counter from NVS ... ");
        //err = nvs_get_i32(my_handle, "restart_counter", &restart_counter);
		size_t name_lenght = 0;
		err = nvs_get_str(my_handle, "myname", NULL, &name_lenght);
		err = nvs_get_str(my_handle, "myname", myname, &name_lenght);
		if(name_lenght>0)
			printf("Unique name %s %d\n", myname, name_lenght);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                //printf("Restart counter = %d\n", restart_counter);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
				snprintf(myname, UID_NAME_LENGHT, "%s%02X%02X%02X%02X%02X%02X", NAME_PREFIX, esp_random()%0xFF, esp_random()%0xFF, esp_random()%0xFF, esp_random()%0xFF, esp_random()%0xFF, esp_random()%0xFF);
                err = nvs_set_str(my_handle, "myname", myname);
				break;
            default :
                printf("Error (%d) reading!\n", err);
				snprintf(myname, UID_NAME_LENGHT, "%s%02X%02X%02X%02X%02X%02X", NAME_PREFIX, esp_random()%0xFF, esp_random()%0xFF, esp_random()%0xFF, esp_random()%0xFF, esp_random()%0xFF, esp_random()%0xFF);
				err = nvs_set_str(my_handle, "myname", myname);
				break;
        }
		printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        
		printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        nvs_close(my_handle);
    }
	
	printf("Unique name %s\n", myname);

	// build the imprinting string
	snprintf(imprinting, imprinting_size, "{\"name\":\"%s\",\"xpub\":\"%s\"}", myname, UID_getTpub());
	printf("INIT: %s", imprinting);

	// start the the thread that updates the
	// contracts cache from the block-chain
	// here we are using pthread, but is up to the user of the library
	// to chose how to schedule the execution of UID_getContracts()
    xTaskCreatePinnedToCore(updater, "updater", 1024*8, NULL, 1, NULL, 1);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	printf("---- Starting message loop\n");	
	while(1){
		xQueueReceive(provider_queue, &main_in, portMAX_DELAY);
		service_provider(&main_in);
		printf("---- WaitMsg\n");
	}
}
