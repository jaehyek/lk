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

#ifndef _PLR_EMMC_POFF_H_
#define _PLR_EMMC_POFF_H_

#include "plr_common.h"
#include "plr_hooking.h"
#include "plr_emmc_internal.h"
#include <mmc_wrapper.h>

int process_attach_poweroff(struct processing_manage *pro_manage, struct sdhci_host *host);

#endif
