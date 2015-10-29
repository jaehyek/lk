#ifndef __LGE_BL_UNLOCK_H
#define __LGE_BL_UNLOCK_H

typedef enum {
    OEM_UNLOCK_DISABLED = 0,
    OEM_UNLOCK_ENABLED = 1,
    OEM_UNLOCK_NO_PARTITION = 2,
} OemUnlockState;

bool bl_lock_state_flash_allowed(char * entry);
bool bl_unlock_state_flash_allowed(char * entry);
bool bl_unlock_state_erase_allowed(char * entry);

OemUnlockState is_oem_unlock_enable();
int cmd_device_unlock(const char *arg, void *data, unsigned sz);
void cmd_oem_device_id(const char *arg, void *data, unsigned sz);
void bootloader_unlock_erase_mmc(const char *arg);
void bootloader_unlock_erase_key(void);
void display_bl_unlock_info(char *mode, char *data);
void display_bl_unlock_info_fastboot_message(char *mode, char *data);
void goto_unlock_completed_mode(void);
void goto_unlock_incomplete_mode(void);

#endif
