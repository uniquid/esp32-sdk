#include "global_var.h"

bool apFlag = false;
QueueHandle_t provider_queue;

uint16_t imprinting_size = 1024;
char imprinting[1024] = {0};
char myname[UID_NAME_LENGHT] = {0};

static char address[18] = "X@$!!";
static uint8_t mac[6];
uint8_t *getMacAddress(int fake)
{
    FILE *fd = NULL;

    fd = fopen("/sys/class/net/eth0/address", "r");
    fgets(address, sizeof(address), fd);
    fclose(fd);
    sscanf(address, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", mac+0, mac+1, mac+2, mac+3, mac+4, mac+5 );
    if(fake) // use fake address
    {   // try to read serial.no
        int uniq = open("serial.no", O_RDWR|O_CREAT, 0644);
        if (read(uniq, mac, sizeof(mac)) != sizeof(mac)) // if we cant read userial.no generate one
        {
            int rnd = open("/dev/random", O_RDONLY);
            if(read(rnd, mac, sizeof(mac)) <= 0) // if we cant read /dev/random use time for seed
                *(int32_t *)mac = time(NULL);
            close(rnd);
            write(uniq, mac, sizeof(mac));
        }
        close(uniq);
    }
    return mac;
}
