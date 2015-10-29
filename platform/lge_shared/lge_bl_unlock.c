#ifdef LGE_BOOTLOADER_UNLOCK
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <lge_bl_unlock.h>
#include <lge_ftm.h>
#include <partition_parser.h>

enum {
    UNLOCK_ERROR_NONE                 = 1000,
    UNLOCK_ERROR_NO_PARTITION         = 1001,
    UNLOCK_ERROR_MMC_READ             = 1002,
    UNLOCK_ERROR_NO_MEMORY            = 1003,
    UNLOCK_ERROR                      = 1004,
    UNLOCK_ERROR_NONE_ENABLE          = 1005,
    UNLOCK_ERROR_NONE_DISABLE         = 1006,
    UNLOCK_ERROR_UNKNOWN_STATUS       = 1007,
    UNLOCK_ERROR_MAX,
};

static char *LOCKED_FLASH_ALLOWED_PTN[] = {
        "unlock",
        NULL };

static char *UNLOCKED_FLASH_ALLOWED_PTN[] = {
        "boot",
        "recovery",
        "system",
        NULL };

static char *UNLOCKED_ERASE_ALLOWED_PTN[] = {
        "boot",
        "recovery",
        "system",
        "userdata",
        "cache",
        NULL };

static bool check_list(char**list, char* entry)
{
    int i = 0;
    if(list == NULL || entry == NULL)
        return false;

    while(*list != NULL)
    {
        if(!strcmp(entry, *list))
            return true;

        list++;
    }

    return false;
}

bool bl_lock_state_flash_allowed(char * entry)
{
    return check_list(LOCKED_FLASH_ALLOWED_PTN, entry);
}

bool bl_unlock_state_flash_allowed(char * entry)
{
    return check_list(UNLOCKED_FLASH_ALLOWED_PTN, entry);
}

bool bl_unlock_state_erase_allowed(char * entry)
{
    return check_list(UNLOCKED_ERASE_ALLOWED_PTN, entry);
}

OemUnlockState is_oem_unlock_enable()
{
    int ret = UNLOCK_ERROR;
    unsigned long long ptn = 0;
    unsigned long long size = 0;
    int index = INVALID_PTN;
    char *output = NULL;
    uint32_t blocksize;

    index = partition_get_index("persistent");
    ptn = partition_get_offset(index);

    if(ptn == 0)
    {
        dprintf(CRITICAL, "/persistent partition table doesn't exist\n");
        ret = UNLOCK_ERROR_NO_PARTITION;
        goto err;
    }

    mmc_set_lun(partition_get_lun(index));

    size = partition_get_size(index);
    blocksize = mmc_get_device_blocksize();

    output = (char *) malloc(sizeof(char) * blocksize);
    if (output == NULL)
    {
        dprintf(CRITICAL, "Check OEM Unlock : No memory!\n");
        ret = UNLOCK_ERROR_NO_MEMORY;
        goto err;
    }

    if(mmc_read((ptn + size - blocksize), (uint32_t *)output, blocksize))
    {
        dprintf(CRITICAL, "mmc read failure /persistent\n");
        ret = UNLOCK_ERROR_MMC_READ;
        goto err;
    }

    if(output[blocksize-1] == 0x01)
    {
        ret = UNLOCK_ERROR_NONE_ENABLE;
    }
    else if (output[blocksize-1] == 0x00)
    {
        ret = UNLOCK_ERROR_NONE_DISABLE;
    }
    else
    {
        ret = UNLOCK_ERROR_UNKNOWN_STATUS;
    }

err:
    if (output != NULL)
    {
        free(output);
        output = NULL;
    }

    if (ret == UNLOCK_ERROR_NO_PARTITION)
    {
        return OEM_UNLOCK_NO_PARTITION;
    }
    else if (ret == UNLOCK_ERROR_NONE_ENABLE)
    {
        return OEM_UNLOCK_ENABLED;
    }
    else
    {
        return OEM_UNLOCK_DISABLED;
    }
}

void bootloader_unlock_erase_mmc(const char *arg)
{
    unsigned long long ptn = 0;
    unsigned long long size = 0;
    int index = INVALID_PTN;
    uint8_t lun = 0;

    index = partition_get_index(arg);
    ptn = partition_get_offset(index);
    size = partition_get_size(index);

    if(ptn == 0) {
        dprintf(CRITICAL, "Partition table doesn't exist\n");
        return;
    }

    lun = partition_get_lun(index);
    mmc_set_lun(lun);

    if (mmc_erase_card(ptn, size)) {
        dprintf(CRITICAL, "failed to erase partition\n");
        return;
    }
}

void bootloader_unlock_erase_key(void)
{
    unsigned char unlock_certification[LGFTM_BLUNLOCK_KEY_SIZE];
    memset(unlock_certification, 0x00, sizeof(unlock_certification));

    if(ftm_set_item(LGFTM_BLUNLOCK_KEY, unlock_certification) <0)
    {
        dprintf(CRITICAL, "failed to erase bootloader unlock key");
        return;
    }
}

void display_unlock_fastboot_message()
{
    fbcon_puts("-----------------------------------------------------------------\n");
    fbcon_puts("* Welcome to Fastboot Mode for bootloader unlock :\n");
    fbcon_puts("\n");
    fbcon_puts("* Quick Guidance\n");
    fbcon_puts("   1) Read Device ID : fastboot oem device-id \n");
    fbcon_puts("   2) Write Unlock Key : fastboot flash unlock unlock.bin\n");
    fbcon_puts("   3) Check bootloader is unlocked : fastboot getvar unlocked\n");
    fbcon_puts("\n");
    fbcon_puts("* HOW TO exit fastboot mode\n");
    fbcon_puts("   1) Press the Power button for 10 secs to turn off the device\n");
    fbcon_puts("   2) Use 'fastboot reboot' command if the fastboot is available\n");
    fbcon_puts("\n");
    fbcon_puts("-----------------------------------------------------------------\n");
    return;
}

void display_fastboot_message()
{
    fbcon_puts("-----------------------------------------------------------------\n");
    fbcon_puts("* Welcome to Fastboot Mode :\n");
    fbcon_puts("\n");
    fbcon_puts("* Quick Guidance\n");
    fbcon_puts("   1) flash : fastboot flash <partition> <filename>\n");
    fbcon_puts("   2) erase : fastboot erase <partition>\n");
    fbcon_puts("   3) reboot : fastboot reboot OR fastboot reboot-bootloader\n");
    fbcon_puts("\n");
    fbcon_puts("* HOW TO exit fastboot mode\n");
    fbcon_puts("   1) Press the Power button for 10 secs to turn off the device\n");
    fbcon_puts("   2) Use 'fastboot reboot' command if the fastboot is available\n");
    fbcon_puts("\n");
    fbcon_puts("-----------------------------------------------------------------\n");
    return;
}

void display_bl_unlock_info(char *mode, char *data)
{
    fbcon_puts( "--------------------------------------------\n");
    fbcon_puts("\n");
    if(!strncmp(mode, "success", strlen(mode)))
    {
        fbcon_puts(data);
    }
    else if(!strncmp(mode, "error", strlen(mode)))
    {
        fbcon_puts( "Unlock Bootloader Error!\n");
        fbcon_puts(data);
        fbcon_puts("\n");
    }
    else
    {
        fbcon_puts(data);
    }
    fbcon_puts( "\n");
    fbcon_puts( "--------------------------------------------\n");
    return;
}

void display_bl_unlock_info_fastboot_message(char *mode, char *data)
{
    char response[128];
    memset(response, 0x00, sizeof(response));
    fastboot_info("-----------------------------------------------------------");
    if(!strncmp(mode, "success", strlen(mode)))
    {
        snprintf(response, sizeof(response), data);
        fastboot_info(response);
    }
    else if(!strncmp(mode, "error", strlen(mode)))
    {
        fastboot_info( "Error!!");
        snprintf(response, sizeof(response), data);
        fastboot_info(response);
    }
    else
    {
        snprintf(response, sizeof(response), data);
        fastboot_info(response);
    }
    fastboot_info( "-----------------------------------------------------------\n");

    if(!strncmp(mode, "success", strlen(mode)))
    {
        fastboot_okay("");
    }
    else
    {
        fastboot_fail("");
    }
    return;
}

void cmd_oem_device_id(const char *arg, void *data, unsigned sz)
{
    int ret = -1;
#define DEVICEID_INPUT_SIZE 32
#define DEVICEID_OUTPUT_SIZE 64

    char deviceid_buffer[DEVICEID_OUTPUT_SIZE+1];
    char response[DEVICEID_OUTPUT_SIZE+1];
    memset(deviceid_buffer, 0x00, sizeof(deviceid_buffer));
    memset(response, 0x00, sizeof(response));

    ret = get_deviceid(deviceid_buffer);
    if(ret != 0)
    {
        display_bl_unlock_info_fastboot_message("error", "Device-ID read Fail!");
        return;
    }

    fastboot_info("---------------------------------------------------------");
    fastboot_info("Device-ID");
    memcpy(response, deviceid_buffer, DEVICEID_INPUT_SIZE);
    fastboot_info(response);
    memset(response, 0x00, sizeof(response));
    memcpy(response, &deviceid_buffer[DEVICEID_INPUT_SIZE], DEVICEID_INPUT_SIZE);
    fastboot_info(response);
    fastboot_info("---------------------------------------------------------");
    fastboot_okay("");
}

int cmd_device_unlock(const char *arg, void *data, unsigned sz)
{
    int ret = -1;
    char unlock_certification[LGFTM_BLUNLOCK_KEY_SIZE];
    memset(unlock_certification, 0x00, sizeof(unlock_certification));

    if(sz != LGFTM_BLUNLOCK_KEY_SIZE)
    {
        display_bl_unlock_info_fastboot_message("error", "wrong unlock key size");
        return -1;
    }

    memcpy(unlock_certification, data, LGFTM_BLUNLOCK_KEY_SIZE);
    ret = ftm_set_item(LGFTM_BLUNLOCK_KEY, unlock_certification);

    if (ret < 0)
    {
        display_bl_unlock_info_fastboot_message("error", "Bootloader Unlock key write fail");
        return -1;
    }
    return 0;
}

void goto_unlock_completed_mode(void)
{
    display_fastboot_message();
    return;
}

void goto_unlock_incomplete_mode(void)
{
    display_unlock_fastboot_message();
    return;
}
#endif