#include "plr_lk_wrapper.h"

extern unsigned int calculate_crc32(unsigned char *buffer, int len);
int readline (const char *const prompt)
{
	return read_line(console_buffer, CONFIG_SYS_CBSIZE);
}

int run_command (const char *cmd, int flag)
{
	if(!strcmp(cmd, "reset")) {
		reboot_device(0U);
	}

	return 0;
}

unsigned long rtc_get_time(void)
{
	return qtimer_current_time()/1000;	// unit of qtimer = 1ms
}

/*
uint32_t crc32(uint crc, const uchar *p, int len)
{
	return calculate_crc32((uchar*)p, len);
}
*/
ulong get_timer (ulong base)
{
	return (ulong)qtimer_get_phy_timer_cnt();
}
