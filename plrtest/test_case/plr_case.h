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

#ifndef _PLR_CASE_H_
#define _PLR_CASE_H_

#define PAGE_PER_GB				262144LL	 		// 1024*1024/4
#define PAGE_PER_MB				256LL				// 1024/4
#define SECONDS_PER_DAY 		86400LL 			// 3600 * 24
#define SECONDS_PER_HOUR		3600LL
#define SECONDS_PER_MINUTE		60LL

/*------------------------------------------------------------------------------------------*/

int initialize_daxx_0000( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_daxx_0001( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_daxx_0002( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_daxx_0003_4( uchar * buf, uint test_start_sector, uint test_sector_length );

//@sj 150805 {
int initialize_dummy( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_daxx_0008( uchar * buf, uint test_start_sector, uint test_sector_length );
//@sj 150805 }


int initialize_dpin_0000( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_dpin_0001( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_dpin_0002( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_dpin_0003( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_dpin_0004( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_dpin_0005( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_dpin_0006( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_dpin_0007( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_dpin_0008( uchar * buf, uint test_start_sector, uint test_sector_length );


int initialize_dpin_0008( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_dpin_0009_12( uchar * buf, uint test_start_sector, uint test_sector_length );

int initialize_ddxx_0000( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_ddxx_0001( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_ddxx_0002( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_ddxx_0003( uchar * buf, uint test_start_sector, uint test_sector_length );
int initialize_ddxx_0004( uchar * buf, uint test_start_sector, uint test_sector_length );

/*------------------------------------------------------------------------------------------*/

int read_daxx_0000( uchar * buf, uint test_start_sector, uint test_sector_length);
int read_daxx_0001( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_daxx_0002( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_daxx_0003_4( uchar * buf, uint test_start_sector, uint test_sector_length );

//@sj 150805 {
int read_dummy( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_daxx_0008( uchar * buf, uint test_start_sector, uint test_sector_length );
//@sj 150805 }



int read_dpin_0000( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0001( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0002( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0003( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0004( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0005( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0006( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0007( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0008( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0008( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_dpin_0009_12( uchar * buf, uint test_start_sector, uint test_sector_length );

int read_ddxx_0000( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_ddxx_0001( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_ddxx_0002( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_ddxx_0003( uchar * buf, uint test_start_sector, uint test_sector_length );
int read_ddxx_0004( uchar * buf, uint test_start_sector, uint test_sector_length );

/*------------------------------------------------------------------------------------------*/

int write_daxx_0000( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_daxx_0001( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_daxx_0002( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_daxx_0003_4( uchar * buf, uint test_start_sector, uint test_sector_length );

//@sj 150805 {
int write_dummy( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_daxx_0008( uchar * buf, uint test_start_sector, uint test_sector_length );
//@sj 150805 }

int write_dpin_0000( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0001( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0002( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0003( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0004( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0005( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0006( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0007( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0008( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0008( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_dpin_0009_12( uchar * buf, uint test_start_sector, uint test_sector_length );

int write_ddxx_0000( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_ddxx_0001( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_ddxx_0002( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_ddxx_0003( uchar * buf, uint test_start_sector, uint test_sector_length );
int write_ddxx_0004( uchar * buf, uint test_start_sector, uint test_sector_length );

/*------------------------------------------------------------------------------------------*/

#endif
