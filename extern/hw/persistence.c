#include <stdlib.h>
#include <stdio.h>
#include "UID_bchainBTC.h"

extern cache_buffer *current, *secondb;

char *load_tprv(char *privateKey, size_t size)
{
    privateKey = NULL;
    return privateKey;
}
void store_tprv(char *privateKey)
{
    //privateKey = NULL;
    return;
}

void store_contracts(char * name, uint8_t *contracts, size_t size)
{
    return;
}

void load_contracts(cache_buffer ** cache){
    *cache = current;
    (current->validCacheEntries)=0;
    (current->validClientEntries)=0; 
}
