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

//
// Note : Calibration stuffs are fully re-designed by SJ in July ~ June '15.
//


#ifndef _PLR_CALIBRATION_H_
#define _PLR_CALIBRATION_H_


#include "plr_emmc_internal_state.h"				//@sj 150702 to use processing_phase_e

typedef struct {
	s32						eod;					// tick for End of DMA
	s32 					eob;					// tick for End of BUSY
	s32 					eof;					// tick for End of CACHE FLUSH
} tick_s;

//
// Note: Now all the published calibration functions have prefix calib_
//

void 	calib_init(void);
int 	calib_make_table(void);
int 	calib_generate_internal_po_time(uint req_size);
void 	calib_print_table(void);
void 	calib_calc_ticks_min_max(s32 cnk_siz, tick_s* t);

extern processing_phase_e calib_get_last_calib_phase(void); //@sj 150702, used for calibration time calc


//
// Helper functions for cache flush
//

void calib_add_cached_write_data_size(int data_size);
uint calib_get_cached_write_size_before_flush(void);
void calib_reset_cached_write_data_size(void);

//
// Calibration common
//

int send_internal_calibration_info(int action, bool cal_test_enable); // @sj 150708

#endif
