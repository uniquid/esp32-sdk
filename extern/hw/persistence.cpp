#include <stdlib.h>
#include <stdio.h>
#include "UID_bchainBTC.h"
#include "src/multi_defines.h"

#include "FS.h"
#include "SD.h"
#include "SPI.h"

uint8_t const SS_PIN = SS;
uint8_t const MOSI_PIN = MOSI;
uint8_t const MISO_PIN = MISO;
uint8_t const SCK_PIN = SCK;
static size_t log_size=0;

extern "C" {
    void store_contracts(char* name, uint8_t * contracts, size_t size);
    void load_contracts(cache_buffer ** cache);
    extern cache_buffer* current;
    extern cache_buffer* secondb;
};

char sd_mount(char * data)
{ 
    bool msd = false;
    char timeout = 0;
	while(!msd){
        printf(">>> timeout %d\n", timeout);
        if(!(msd = SD.begin())){
            printf("Card Mount Failed\n");
            timeout++;
            if(timeout>2){
                data = NULL;
                return 0;
            }
        }
    }
    
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        printf("No SD card attached\n");
        data = NULL;
        return 0;
    }
    printf("SD Card Type: ");
    if(cardType == CARD_MMC){
        printf("MMC\n");
    } else if(cardType == CARD_SD){
        printf("SDSC\n");
    } else if(cardType == CARD_SDHC){
        printf("SDHC\n");
    } else {
        printf("UNKNOWN\n");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    printf("SD Card Size: %lluMB\n", cardSize);
    return 1;
}

char sd_removefile(char * filename)
{ 
    bool msd = false;
    char timeout = 0;
	while(!msd){
        printf(">>> timeout %d\n", timeout);
        if(!(msd = SD.begin())){
            printf("Card Mount Failed\n");
            timeout++;
            if(timeout>2){
                filename = NULL;
                return 0;
            }
        }
    }
    
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        printf("No SD card attached\n");
        filename = NULL;
        return 0;
    }
    printf("SD Card Type: ");
    if(cardType == CARD_MMC){
        printf("MMC\n");
    } else if(cardType == CARD_SD){
        printf("SDSC\n");
    } else if(cardType == CARD_SDHC){
        printf("SDHC\n");
    } else {
        printf("UNKNOWN\n");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    printf("SD Card Size: %lluMB\n", cardSize);
    SD.remove(filename);
    return 1;
}

char *load_tprv(char *privateKey, size_t size)
{
    if(!sd_mount(privateKey))
        return NULL;

    File file = SD.open("/tp");
    if(!file){
        printf("Failed to open file for reading\n");
        privateKey = NULL;
        return NULL;
    }

	char * ikey = privateKey;
    while(file.available()){
        *ikey = (char)(file.read());
		ikey++;
    }
    *ikey = 0;
    if(strlen(privateKey)!=0){
		printf("text: %s\nsize: %d\n", privateKey, strlen(privateKey));
    }
    else{
        file.close();
        privateKey = NULL;
        return NULL;
    }
    file.close();
    return privateKey;
}
void store_tprv(char *privateKey)
{
    if(!sd_mount(privateKey))
        return;

	printf("Writing this %s in file: %s\n", privateKey, "/tp");

    File file = SD.open("/tp", "w+");
    if(!file){
        printf("Failed to open file for writing\n");
        privateKey = NULL;
        return;
    }
    if(file.print(privateKey)){
        printf("File written\n");
    } else {
        printf("Write failed\n");
        privateKey = NULL;
    }
    file.close();
    return;
}

char *load_vin(char *vin, size_t size)
{
    if(!sd_mount(vin))
        return NULL;

    File file = SD.open("/vin");
    if(!file){
        printf("Failed to open file for reading\n");
        vin = NULL;
        return NULL;
    }

	char * ikey = vin;
    while(file.available()){
        *ikey = (char)(file.read());
		ikey++;
    }
    *ikey = 0;
    if(strlen(vin)!=0){
		printf("text: %s\nsize: %d\n", vin, strlen(vin));
    }
    else{
        file.close();
        vin = NULL;
        return NULL;
    }
    file.close();
    return vin;
}

void store_vin(char *vin)
{
    if(!sd_mount(vin))
        return;

	printf("Writing this %s in file: %s\n", vin, "/vin");

    File file = SD.open("/vin", "w+");
    if(!file){
        printf("Failed to open file for writing\n");
        vin = NULL;
        return;
    }
    if(file.print(vin)){
        printf("File written\n");
    } else {
        printf("Write failed\n");
        vin = NULL;
    }
    file.close();
    return;
}


void store_contracts(char * name, uint8_t *contracts, size_t size)
{
    if(!sd_mount(NULL))
        return;
    
	printf("Writing %s in file: %s\n", "contracts", name);

    File file = SD.open(name, "w+");
    if(!file){
        printf("Failed to open file for writing\n");
        return;
    }
    if(size == file.write(contracts, size)){
        printf("File written\n");
    } else {
        printf("Write failed\n");
    }
    file.close();
    return;
}

void load_contracts(cache_buffer ** cache){
    *cache = current;
    (current->validCacheEntries)=0;
    (current->validClientEntries)=0;

    if(!sd_mount(NULL))
        return;

    File file = SD.open("/ccache.bin");
    if(file){
        (current->validCacheEntries)=0;
        while(file.available()){
            while ( sizeof(UID_SecurityProfile) == file.read((uint8_t*)((current->contractsCache) + (current->validCacheEntries)), sizeof(UID_SecurityProfile)) )
                (current->validCacheEntries)++;
        }
        file.close();
    }
    else {
        printf("Failed to open file for reading\n");
    }

    file = SD.open("/clicache.bin");
    if(file){
        (current->validClientEntries)=0;
        while(file.available()){
            while ( sizeof(UID_ClientProfile) == file.read((uint8_t*)((current->clientCache) + (current->validClientEntries)), sizeof(UID_ClientProfile)) )
                (current->validClientEntries)++;
        }
        file.close();
    }
    else {
        printf("Failed to open file for reading\n");
    }
}

void logger_save(char *log)
{
    size_t written = 0;

    if(!sd_mount(log))
        return;

	printf("Writing this %s in file: %s\n", log, "/log");

    File file = SD.open("/log", "a+");
    log_size = file.size();
    if(!file){
        printf("Failed to open file for writing\n");
        log = NULL;
        return;
    }
    if(written = file.print(log)){
        printf("File written\n");
    } else {
        printf("Write failed\n");
        log = NULL;
    }
    log_size += written;
    file.close();
    return;
}

void logger_load(char *log)
{
    char logfile[] = "/log";

    if(!sd_mount(log))
        return;

    File file = SD.open(logfile);
    if(!file){
        printf("Failed to open file for reading\n");
        log = NULL;
        return;
    }

	char * ikey = log;
    *ikey = '[';
    ikey++;
    while(file.available()){
        *ikey = (char)(file.read());
		ikey++;
    }
    ikey--;
    *ikey = ']';
	ikey++;
    *ikey = 0;
    if(strlen(log)!=0){
		printf("text: %s\nsize: %d\n", log, strlen(log));
    }
    else{
        file.close();
        log = NULL;
        return;
    }
    file.close();
    sd_removefile(logfile);
    log_size = 0;
    return;
}

void logger_delete()
{
    char log[5] = {0};
    char logfile[] = "/log";

    if(!sd_mount(log))
        return;

    sd_removefile(logfile);
    log_size = 0;
    return;
}

void init_log_size(){
    char * ptr;
    if(!sd_mount(ptr))
        return;

    File file = SD.open("/log", "r");
    if(!file){
        printf("Failed to open file for writing\n");
        ptr = NULL;
        return;
    }
    log_size = file.size();
    printf("FILE SIZE: %d\n", log_size);
    file.close();
    return;
}

size_t check_if_hacked(){
    return log_size;
}