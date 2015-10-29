#include <lib/console.h>
#include <platform.h>
#include <string.h>
#include <qtimer.h>

#define CONFIG_SYS_CBSIZE 256

#define MMC_CARD(host) (&container_of(host, struct mmc_device, host)->card)

char console_buffer[CONFIG_SYS_CBSIZE + 1];
int readline (const char *const prompt);
ulong get_timer(ulong base);
