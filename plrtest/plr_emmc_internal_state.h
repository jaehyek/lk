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

#ifndef _PLR_EMMC_POWEROFF_STATE_H_
#define _PLR_EMMC_POWEROFF_STATE_H_

#include "plr_type.h"

// joys,2015.06.19 ------------------
typedef enum {
	INTERNAL_ACTION_NONE,
	INTERNAL_ACTION_TIMING_CALIBRATION,
	INTERNAL_ACTION_POWEROFF
} internal_action_e;

typedef enum {
	PROCESS_PHA_DATA_TRANS,
	PROCESS_PHA_WRITE_BUSY,
	PROCESS_PHA_CACHE_FLUSH,
	PROCESS_PHA_ERASE,
	PROCESS_PHA_SANITIZE,
	PROCESS_PHA_MAX
} processing_phase_e;

typedef enum {
	POFF_TYPE_TIMING,
	POFF_TYPE_DELAY
} poweroff_type_e;

typedef struct _poweroff_info_ {
	int b_internal;								// 0 : external, 1 : internal
	int b_poff_require;							// Require poweroff
	u64 poff_value;								// poweroff threshold, @sj 150703
	processing_phase_e 	poff_phase;				// MMC_PROCESSING_POSISION
	poweroff_type_e 	poff_type;				// POWEROFF_TYPE
} poweroff_info_t;

typedef struct _internal_info_ {
	int action;						// current save or calibration or internal poweroff
	uint last_poff_lsn;				// Read only. Do not change by upper user
	poweroff_info_t poff_info;		// poweroff information
} internal_info_t;
// ---------------------------------

#endif
