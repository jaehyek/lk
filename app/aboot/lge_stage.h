#ifndef _GOTA_BOOTLOADER_STAGE_H
#define _GOTA_BOOTLOADER_STAGE_H

struct gota_stage_message {
    char command[32];
    char status[32];
    char recovery[768];
    char stage[32];
    char reserved[224];
};

unsigned int LGE_bootloader_Last_stage (void);
int LGE_clear_bootloader_message(void);
int LGE_get_bootloader_message(struct gota_stage_message *out);
int LGE_set_bootloader_message(const struct gota_stage_message *in);

#endif