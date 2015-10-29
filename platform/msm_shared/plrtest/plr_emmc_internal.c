 /*
  * Copyright (c) 2015	ElixirFlash Technology Co., Ltd.
  *  http://www.elixirflash.com/
  *
  * Powerloss Test for U-boot
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License version 2 as
  * published by the Free Software Foundation.
  */

#include "plr_common.h"
#include "plr_emmc_internal.h"
#include "plr_emmc_calibration.h"
#include "plr_emmc_poff.h"
#include "plr_emmc_internal_state.h"
#include "plr_lk_wrapper.h"
#include <mmc_wrapper.h>

s64 _ticks_sod;		// start of dma

/***************************************************************************************************//**
*
* @brief	internal action call functions.
*
*******************************************************************************************************/
static int internal_process_attach(struct processing_manage *pro_manage, struct sdhci_host *host)
{
	int ret = 0;
	struct mmc_card *card = MMC_CARD(host);
	internal_info_t *int_info = &card->internal_info;

	switch (int_info->action)
	{
		// Calibration
		case INTERNAL_ACTION_TIMING_CALIBRATION:
			process_attach_calibration(pro_manage, host);
			break;

		// Internal poweroff
		case INTERNAL_ACTION_POWEROFF:
			process_attach_poweroff(pro_manage, host);
			break;

		default:
			ret = -1;
			break;
	}

	return ret;
}

/***************************************************************************************************//**
*
* @brief	internal phase call functions.
*
*******************************************************************************************************/
static int internal_processing(
	struct processing_manage *pro_manage, struct sdhci_host *host, uint cmd_arg, uint blocks, processing_phase_e cur_pos)
{
	int ret = 0;
	struct mmc_card *card = MMC_CARD(host);
	internal_info_t *int_info = &card->internal_info;
#if 1
	if (int_info->action == INTERNAL_ACTION_TIMING_CALIBRATION ||
		int_info->action == INTERNAL_ACTION_POWEROFF) {

		if (cur_pos == PROCESS_PHA_DATA_TRANS)
		{
			_ticks_sod = get_tick_count64();	// @sj 150703
		}
	}
#endif
	if (int_info->action == INTERNAL_ACTION_POWEROFF)
	{
		if (int_info->poff_info.b_poff_require == FALSE)
			goto out;

		if (int_info->poff_info.poff_type == POFF_TYPE_TIMING)
		{
			if (int_info->poff_info.poff_phase == PROCESS_PHA_DATA_TRANS ||
				int_info->poff_info.poff_phase == PROCESS_PHA_WRITE_BUSY) {

				if (cur_pos == PROCESS_PHA_DATA_TRANS || cur_pos == PROCESS_PHA_WRITE_BUSY)
				{
					//nothing
				}
				else if (int_info->poff_info.poff_phase != cur_pos)
					goto out;

			}
			else if (int_info->poff_info.poff_phase != cur_pos)
				goto out;
		}
		else if (int_info->poff_info.poff_phase != cur_pos)
			goto out;
	}

	if (pro_manage->processing_phase_request[cur_pos])
		ret = pro_manage->processing_phase_request[cur_pos](host, cmd_arg, blocks);

	if ( ret == -(INTERNAL_POWEROFF_FLAG) ) {
		memset(&int_info->poff_info, 0, sizeof(poweroff_info_t));
//		memset(int_info, 0, sizeof(internal_info_t));
	}

out:
	return ret;

}

/***************************************************************************************************//**
*
* @brief	User Entry function
*
*******************************************************************************************************/

int internal_processing_on_mmc(struct sdhci_host *host, uint cmd_arg, uint blocks, processing_phase_e cur_pos)
{
	int ret = 0;
	struct processing_manage pro_manage;

	ret = internal_process_attach(&pro_manage, host);

	if (ret)
		return 0;

	ret = internal_processing(&pro_manage, host, cmd_arg, blocks, cur_pos);

	return ret;
}
