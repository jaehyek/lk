#include <debug.h>
#include <string.h>

#include <dev/flash.h>
#include <partition_parser.h>
#include <mmc.h>

#include "lge_stage.h"

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

unsigned int LGE_bootloader_Last_stage (void)
{
    struct gota_stage_message msg;
    if (LGE_get_bootloader_message(&msg))
    {
        dprintf(INFO,"########################### Fail Get Misc ########################### \n");
        return 0;
    }
    msg.stage[sizeof(msg.stage)-1] = '\0'; //Ensure termination
    if (!strcmp("S-3/3",msg.stage))
    {
        return 1;
    }
     return 0;
}

int LGE_clear_bootloader_message(void)
{
    int clear_stage = 0;
    struct gota_stage_message msg;
    memset(&msg, 0, sizeof(msg));
    clear_stage = LGE_set_bootloader_message(&msg);
    return clear_stage;
}

int LGE_get_bootloader_message(struct gota_stage_message *in)
{
    char *ptn_name = "misc";
    unsigned long long ptn = 0;
    unsigned int size = ROUND_TO_PAGE(sizeof(*in),511);
    unsigned char data[size];
    int index = INVALID_PTN;

    index = partition_get_index((const char *) ptn_name);
    ptn = partition_get_offset(index);
    if(ptn == 0) {
            dprintf(CRITICAL,"partition %s doesn't exist\n",ptn_name);
            return -1;
    }
    if (mmc_read(ptn , (unsigned int*)data, size)) {
            dprintf(CRITICAL,"mmc read failure %s %d\n",ptn_name, size);
            return -1;
    }
    memcpy(in, data, sizeof(*in));
    return 0;
}

int LGE_set_bootloader_message(const struct gota_stage_message *out)
{
    char *ptn_name = "misc";
    unsigned long long ptn = 0;
    unsigned int size = ROUND_TO_PAGE(sizeof(*out),511);
    unsigned char data[size];
    int index = INVALID_PTN;

    index = partition_get_index((const char *) ptn_name);
    ptn = partition_get_offset(index);
    if(ptn == 0) {
            dprintf(CRITICAL,"partition %s doesn't exist\n",ptn_name);
            return -1;
    }
    memcpy(data, out, sizeof(*out));
    if (mmc_write(ptn , size, (unsigned int*)data)) {
            dprintf(CRITICAL,"mmc write failure %s %d\n",ptn_name, sizeof(*out));
            return -1;
    }
    return 0;
}
