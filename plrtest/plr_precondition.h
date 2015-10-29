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

#ifndef _PLR_PRECONDITION_H_
#define _PLR_PRECONDITION_H_

int plr_prepare(  uint test_start_sector,
				  uint test_sector_length,
				  bool bdirty_fill);

void precondition_by_protocol(void);

#endif
