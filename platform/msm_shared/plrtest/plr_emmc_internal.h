
#ifndef _PLR_EMMC_INTERNAL_H_
#define _PLR_EMMC_INTERNAL_H_

#include "plr_common.h"
#include "plr_hooking.h"
#include "plr_lk_wrapper.h"
#include "plr_emmc_internal_state.h"
#include <mmc_wrapper.h>

struct processing_manage {
	int 	(*processing_phase_request[PROCESS_PHA_MAX])(struct sdhci_host *, uint, uint);
};

int internal_processing_on_mmc (struct sdhci_host *host, uint cmd_arg, uint blocks, processing_phase_e cur_pos);
#endif
