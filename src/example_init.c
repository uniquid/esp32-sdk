#include "UID_bchainBTC.h"
#include "UID_fillCache.h"
#include "UID_identity.h"
#include "UID_utils.h"

#include <freertos/task.h>
#include <freertos/ringbuf.h>
char imprinting[500] = {0};

extern cache_buffer *current, *secondb;

void store_contracts(char* name, uint8_t * contracts, size_t size);
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
				if (cacheWrite)
				{
					xSemaphoreTake(OBDaccess, portMAX_DELAY);
					store_contracts("/ccache.bin", (uint8_t *)(cache->contractsCache), sizeof(UID_SecurityProfile)*(cache->validCacheEntries));
					ResetOBD = true;
					//store_contracts("/clicache.bin", (uint8_t *)(cache->clientCache), sizeof(UID_ClientProfile)*(cache->validClientEntries));
					xSemaphoreGive(OBDaccess);
				}
				else
				{
					printf("Skipping cache write\n");
				}
			}
			else {
				// timeout o errori
				ESP_LOGU("INIT", "UID_getContracts() return %d", ret);
			}
			// just print the cache contents
			printCache(cache);
			vTaskDelay(45000);
		} else {
			vTaskDelay(10000);
		}
	}
}

int main(void)
{
	uint16_t quotient = 0, remainder = 0, i = 0;
	char slen[5] = {0};
	data_in_service main_in;
	cache_buffer *cache;
	// set up the URL to insight-api appliance
	UID_pApplianceURL = "http://explorer.uniquid.co:3001/insight-api";
	// set up the URL to the registry appliance
	UID_pRegistryURL = "http://appliance4.uniquid.co:8080/registry";
	// generate or load the identity
	//UID_getLocalIdentity("tprv8ZgxMBicQKsPdysK6szK4heHyavBL4FEfsWj3QAzgQqkJknpsMLksqLeui64sAugRs5YpN3cB8BYUVpjyaY72RgYst3xGB9i7Bt4UcMe9kh");
	//UID_getLocalIdentity("tprv8ZgxMBicQKsPfBdbFwEiJBEmhwoN1QWgWBsAeaRf69P2J9shnJRqP6y173ADVFYvkUW9GLR3yWkY4mhSinxDyCXcYfz1bU7XDaJ8UfvACue");
	UID_getLocalIdentity(NULL);
	load_contracts(&cache);
	printCache(cache);

	// build the imprinting string
	snprintf(imprinting, sizeof(imprinting), "{\"name\":\"%s\",\"xpub\":\"%s\"}", vin, UID_getTpub());
	ESP_LOGU("INIT", "%s", imprinting);

	// start the the thread that updates the
	// contracts cache from the block-chain
	// here we are using pthread, but is up to the user of the library
	// to chose how to schedule the execution of UID_getContracts()
    xTaskCreatePinnedToCore(updater, "updater", 1024*8, NULL, 1, NULL, 1);
	vTaskDelay(1000);
	printf("---- Starting message loop\n");	
	while(1){
		//if(uxQueueMessagesWaiting(provider_queue)){
			xQueueReceive(provider_queue, &main_in, portMAX_DELAY);
			service_provider(&main_in);
		//}
		printf("---- WaitMsg\n");
	}
}
