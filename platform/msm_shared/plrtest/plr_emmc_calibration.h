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

#ifndef _PLR_EMMC_CALIBRATION_H_
#define _PLR_EMMC_CALIBRATION_H_

#include "plr_common.h"
#include "plr_hooking.h"
#include <mmc_wrapper.h>

int process_attach_calibration(struct processing_manage *pro_manage, struct sdhci_host *host);

#endif
