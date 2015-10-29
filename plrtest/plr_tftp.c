/*
 * Copyright (c) 2013  ElixirFlash Technology Co., Ltd.
 *              http://www.elixirflash.com/
 *
 * Powerloss Test for U-boot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
 
#include "plr_common.h"
#include "plr_tftp.h"
#include "plr_protocol.h"
#include "plr_hooking.h"
 
#define MAX_UPLOAD_RETRY_COUNT	5

struct tftp_state g_tftp_state;

void tftp_fw_upgrade(void)
{
#ifdef PLR_TFTP_FW_UP
	int ret = 0;
	char cmd[PLRTOKEN_BUFFER_SIZE] = {0,};
	
	send_token(PLRTOKEN_UTFTP_STATE, NULL);

	switch(g_tftp_state.state){
		case 0:
			send_token(PLRTOKEN_UTFTP_SETTING, NULL);
			
			sprintf(cmd,
				"setenv ipaddr %s; setenv serverip %s; setenv gatewayip %s; setenv netmask %s; setenv ethaddr %s; setenv tftptimeout 10000; saveenv",
				g_tftp_state.client_ip, g_tftp_state.server_ip, g_tftp_state.gateway_ip, g_tftp_state.netmask, g_tftp_state.mac_addr);
			
			ret = run_command(cmd, 0);
			if(ret == -1) {
			 	plr_err("tftp configuration failed\n");
				send_token(PLRTOKEN_ENV_ERROR, NULL);
				ret = run_command("reset", 0);
				return;
			}
			
			send_token(PLRTOKEN_UTFTP_APPLITED, NULL);
			
			ret = run_command("reset", 0);
			break;

		case 1:
			send_token(PLRTOKEN_UFILE_PATH, NULL);
			send_token(PLRTOKEN_UDNLOAD_START, NULL);
			
			sprintf(cmd, "tftpboot 41000000 %s", g_tftp_state.file_path);
			
			ret = run_command(cmd, 0);
			if(ret == -1) {
			 	plr_err("tftp client execution failed\n");
				send_token_param(PLRTOKEN_UDNLOAD_DONE, 1);
				ret = run_command("reset", 0);
				return;
			}

			ret = run_command("movi write u-boot 0 41000000", 0);

			// plr_debug("send token : %d\n", PLRTOKEN_UDNLOAD_DONE);
			send_token_param(PLRTOKEN_UDNLOAD_DONE, 0);

			if(g_tftp_state.option == 0)
				ret = run_command("reset", 0);

			break;
	}
#else
	plr_err("\ntftp f/w upgrade not support!\n");
#endif	// PLR_TFTP_FW_UP
}
