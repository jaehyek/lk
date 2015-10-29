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


#include <config.h>
#include "plr_common.h"
#include "plr_hooking.h"
#include "plr_err_handling.h"
#include <part.h>
#include <asm/u-boot.h>
#include <linux/types.h>
#include <mmc.h>
#include <watchdog.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pinmux.h>

#define UNSTUFF_BITS(resp,start,size)										\
	({																		\
		const int __size = size;											\
		const unsigned long __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);								\
		const int __shft = (start) & 31;									\
		unsigned long __res;												\
																			\
		__res = resp[__off] >> __shft;										\
		if (__size + __shft > 32)											\
			__res |= resp[__off-1] << ((32 - __shft) % 32);					\
		__res & __mask;														\
	})



#ifndef CONFIG_WD_PERIOD
	# define CONFIG_WD_PERIOD	(10 * 1000 * 1000)	/* 10 seconds default*/
#endif

extern struct mmc *find_mmc_device(int dev_num);
extern block_dev_desc_t *get_dev(char* ifname, int dev);
extern int fat_format_device(block_dev_desc_t *dev_desc, int part_no);
extern int mmc_erase(struct mmc *mmc, int part, uint start, uint block);
extern void __udelay (unsigned long usec);

int get_mmc_product_name(uint dev_num, char *name)
{
	struct mmc *mmc;
	uint *resp = NULL;
	char prod_name[8] = {0};

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		return -PLR_ENODEV;
	}

	resp = mmc->cid;

	prod_name[0]	= UNSTUFF_BITS(resp, 96, 8);
	prod_name[1]	= UNSTUFF_BITS(resp, 88, 8);
	prod_name[2]	= UNSTUFF_BITS(resp, 80, 8);
	prod_name[3]	= UNSTUFF_BITS(resp, 72, 8);
	prod_name[4]	= UNSTUFF_BITS(resp, 64, 8);
	prod_name[5]	= UNSTUFF_BITS(resp, 56, 8);
	prod_name[6]	= '\0';

	strcpy(name, prod_name);

	return 0;
}

int get_mmc_cache_enable(uint dev_num)
{
	struct mmc *mmc;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		return -PLR_ENODEV;
	}
	// printf ("cache enable : %d\n", mmc->ext_csd.cache_enable);
	return mmc->ext_csd.cache_ctrl;
}

int get_mmc_packed_enable(uint dev_num)
{
	struct mmc *mmc;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		return -PLR_ENODEV;
	}
	// printf ("cache enable : %d\n", mmc->ext_csd.cache_enable);
	return mmc->ext_csd.packed_event_en;
}

int do_cache_flush(uint dev_num)
{
	struct mmc *mmc;
	int ret = 0;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		 return -PLR_ENODEV;
	}
	ret = mmc->block_dev.block_cache_flush((int)dev_num);

	if (ret)
		return -PLR_EWRITE;
	else
		return 0;
}

int do_cache_ctrl(uint dev_num, uint enable)
{
	struct mmc *mmc;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		 return -PLR_ENODEV;
	}

	return mmc->block_dev.block_cache_ctrl((int)dev_num, enable);
}

// joys,2014.11.18 -----------------------------------------------
int do_mmc_sleep(uint dev_num)
{
	struct mmc *mmc;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		 return -PLR_ENODEV;
	}

	return mmc->block_dev.block_sleep((int)dev_num);
}

int do_mmc_awake(uint dev_num)
{
	struct mmc *mmc;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		 return -PLR_ENODEV;
	}

	return mmc->block_dev.block_awake((int)dev_num);
}
// -----------------------------------------------------------------

// joys,2014.12.01 -------------------------------------------------
int do_mmc_poweroff_notify(uint dev_num, int notify_type)
{
	return emmc_poweroff_notify((int)dev_num, notify_type);
}
// -----------------------------------------------------------------

int do_packed_add_list(int dev_num, ulong start, lbaint_t blkcnt, void*src, int rw)
{
	return emmc_packed_add_list(dev_num, start, blkcnt, src, rw);
}

int do_packed_send_list(int dev_num)
{
	return emmc_packed_send_list(dev_num);;
}

void* do_packed_create_buff(int dev_num, void *buff)
{
	return emmc_packed_create_buff(dev_num, buff);
}

int do_packed_delete_buff(int dev_num)
{
	return emmc_packed_delete_buff(dev_num);
}

struct mmc_packed* do_get_packed_info(int dev_num)
{
	return emmc_get_packed_info(dev_num);
}

uint do_get_packed_max_sectors(int dev_num, int rw)
{
	return emmc_get_packed_max_sectors(dev_num, rw);
}

int do_read(uint dev_num, uchar *data, uint start_sector, uint len)
{
	struct mmc *mmc;
	uint ret = 0;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		return -PLR_ENODEV;
	}
	ret = mmc->block_dev.block_read((int)dev_num, start_sector, (lbaint_t)len, data);

	if (ret == len)
		return 0;
	else{
		plr_info("Read Fail LSN : 0x%08X \n", start_sector);
		return -PLR_EREAD;
	}
}

int do_write(uint dev_num, uchar *data, uint start_sector, uint len)
{
	uint ret = 0;

	struct mmc *mmc;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		return -PLR_ENODEV;
	}

	ret = mmc->block_dev.block_write((int)dev_num, start_sector, (lbaint_t)len, data);

	if (ret == len)
		return 0;
	else {
		plr_info("Writing Failure!!!\n");
		return -PLR_EWRITE;
	}
}

int do_erase(uint dev_num, uint start_sector, uint len)
{
	int ret = 0;
	struct mmc *mmc;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		 return -PLR_ENODEV;
	}

	ret = mmc->block_dev.block_erase ((int)dev_num, start_sector, (lbaint_t)len);
	return (ret == len) ? 0 : 1;
}

/*for smdk5410*/
extern unsigned long mmc_erase_for_checking(int dev_num, unsigned long start, lbaint_t block);
int do_erase_for_checking(uint dev_num, uint start_sector, uint len)
{
	int ret = 0;
	struct mmc *mmc;

	mmc = find_mmc_device((int)dev_num);

	if (!mmc)
	{
		 return -PLR_ENODEV;
	}

	ret = mmc_erase_for_checking((int)dev_num, start_sector, (lbaint_t)len);
	return (ret == len) ? 0 : 1;
}

int make_fat_partition(void)
{
	block_dev_desc_t *dev_desc = get_dev("mmc", SDCARD_DEV_NUM);;

	if (fat_format_device(dev_desc, SDCARD_FAT_PART_NUM) != 0) {
		plr_debug ("SD card Format failure!!!\n");
		return -1;
	}
	return 0;
}

#ifndef name_to_gpio
	#define name_to_gpio(name) simple_strtoul(name, NULL, 10)
#endif

#define GPIO_DELAY mdelay(20);

enum gpio_cmd {
	GPIO_IN,
	GPIO_SET,
	GPIO_CLEAR,
	GPIO_TOGGLE,
};

static int do_mmc_initial(int dev_num)
{
	int err = 0;
	struct mmc *mmc;

	mmc = find_mmc_device(dev_num);

	if (mmc) {
		mmc->has_init = 0;

		err = mmc_init(mmc);

		if (err) {
			printf("no mmc device at slot %x\n", dev_num);
			return err;
		}
	} else {
		printf("no mmc device at slot %x\n", dev_num);
		return 1;
	}

	return 0;
}

// yongja. for hynix eMMC
static int emmc_gpio_clear(int peripheral, int flags)
{
	struct exynos5_gpio_part1 *gpio1 =
		(struct exynos5_gpio_part1 *) samsung_get_base_gpio_part1();
	struct s5p_gpio_bank *bank, *bank_ext;
	int i = 0, bank_ext_bit = 0;

	switch (peripheral) {
	case PERIPH_ID_SDMMC0:
		bank = &gpio1->c0;
		bank_ext = &gpio1->c1;
		bank_ext_bit = 0;
		break;
	case PERIPH_ID_SDMMC1:		// for smdk5410(EFBoard)
		bank = &gpio1->c1;
		bank_ext = &gpio1->d1;
		bank_ext_bit = 4;
		break;
	}

	if (flags & PINMUX_FLAG_8BIT_MODE) {
		for (i = bank_ext_bit; i <= (bank_ext_bit + 3); i++) {
			s5p_gpio_set_pull(bank_ext, i, GPIO_PULL_NONE);
			s5p_gpio_direction_output(bank_ext, i, 0);
		}
	}
	GPIO_DELAY;
	for (i = 0; i < 2; i++) {
		s5p_gpio_set_pull(bank, i, GPIO_PULL_NONE);
		s5p_gpio_direction_output(bank, i, 0);

	}
	GPIO_DELAY;
	for (i = 3; i <= 6; i++) {
		s5p_gpio_set_pull(bank, i, GPIO_PULL_NONE);
		s5p_gpio_direction_output(bank, i, 0);

	}
	GPIO_DELAY;
	return 0;
}

static int emmc_gpio_config(int dev_num, int peripheral, bool b_power)
{
	int err = 0;

	if (b_power) { //GPIO ON
		err = exynos_pinmux_config(peripheral, PINMUX_FLAG_8BIT_MODE);
		if (err) {
			printf("MSHC0 not configured\n");
			return err;
		}
		// Checking dev_num for external poweroff of hynix eMMC bug
		// checking arch/arm/lib/board.c code
		if (dev_num >= 0) {
			mdelay(100);					// for S device
			if (do_mmc_initial(dev_num))
				return -1;
		}
	}
	else { //GPIO OFF
		emmc_gpio_clear(peripheral, PINMUX_FLAG_8BIT_MODE);
	}
	// necessrily
	mdelay(100);
	return 0;
}

static uint tps_pw_tbl[][7] = {
	{ 0, 1, 1, 1, 1, 1, 1},		// 0.5 V
	{ 180, 1, 0, 1, 0, 0, 1}, 	// 1.8 V
	{ 315, 0, 1, 0, 1, 0, 0 },	// 3.15 V
	{ 320, 1, 0, 0, 1, 0, 0 },	// 3.20 V
	{ 325, 0, 0, 0, 1, 0, 0 },	// 3.25 V
	{ 330, 1, 1, 1, 0, 0, 0 },	// 3.30 V
	{ 335, 0, 1, 1, 0, 0 ,0 },	// 3.35 V
	{ 340, 1, 0, 1, 0, 0, 0 },	// 3.40 V
	{ 345, 0, 0, 1, 0, 0, 0 },	// 3.45 V
};

#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif


static int tps7A7200RGW_set_voltage(int voltage)
{
	struct exynos5_gpio_part5 *gpio5 =
        (struct exynos5_gpio_part5 *) samsung_get_base_gpio_part5();
	int i = 0, j = 0;

	/* set eMMC vcc */
	for (i = 0; i < ARRAY_SIZE(tps_pw_tbl); i++) {
		if (tps_pw_tbl[i][0] == voltage) {
			for (j = 0; j < 6; j++) {
				if (tps_pw_tbl[i][j+1] == 0) {
					#ifdef CONFIG_EFBOARD_V02
					s5p_gpio_direction_output(&gpio5->k0, j, 1);
					#else
					if (j == 4)
						s5p_gpio_direction_output(&gpio5->k0, j, 1);
					else
						s5p_gpio_direction_output(&gpio5->k0, j, 0);
					#endif

					#ifndef CONFIG_EFBOARD_V02
					GPIO_DELAY
					#endif
				}
				else {
					#ifdef CONFIG_EFBOARD_V02
					s5p_gpio_direction_output(&gpio5->k0, j, 0);
					#else
					if (j == 4) {
						s5p_gpio_direction_output(&gpio5->k0, j, 0);
					}
					else {
						//memset(&gpio5->k0, 0, sizeof(struct s5p_gpio_bank));
						s5p_gpio_cfg_pin(&gpio5->k0, j, 0);
						s5p_gpio_set_pull(&gpio5->k0, j, GPIO_PULL_NONE);
					}
					#endif

					#ifndef CONFIG_EFBOARD_V02
					GPIO_DELAY
					#endif
				}
			}
			break;
		}
	}
	return 0;
}

extern void s5p_bank_cfg_pin(struct s5p_gpio_bank *bank, int cfg);
extern void s5p_bank_set_value(struct s5p_gpio_bank *bank, int en);

int do_internal_power_control(int dev_num, bool b_power)
{
	/* --------------------------------------------------------
	 * GPK1[0] : VCC
	 * GPK1[1] : RST
	 * GPK1[6] : VCCQ
	 * plr_first is TRUE then GPK1CON[0],[1],[6] OUTPUT
	 * voltage is TRUE then GPK1DAT[0],[1],[6] ENABLE else DISABLE
	 * --------------------------------------------------------*/

	struct exynos5_gpio_part5 *gpio5 =
        (struct exynos5_gpio_part5 *) samsung_get_base_gpio_part5();

	s5p_bank_cfg_pin(&gpio5->k1, 0x1000011);

	if (!b_power)
		s5p_bank_set_value(&gpio5->k1, 0x00);

	GPIO_DELAY

	switch (b_power) {
	case TRUE :
		tps7A7200RGW_set_voltage(335);
		break;
	case FALSE :
		tps7A7200RGW_set_voltage(0);
		break;
	default:
		return -1;
	}

	if (b_power)
		s5p_bank_set_value(&gpio5->k1, 0x43);

	GPIO_DELAY

	emmc_gpio_config(dev_num, PERIPH_ID_SDMMC1, b_power);

	return 0;
}

void emmc_power_control(int voltage)
{
	tps7A7200RGW_set_voltage(voltage);
}

extern void udelay(unsigned long usec);
extern unsigned long rtc_get_tick_count(void);
extern void rtc_reset_tick_count(int enable);
extern void well512_seed(unsigned int nSeed);
extern unsigned int well512_rand(void);
extern unsigned long get_timer(unsigned long base);

unsigned long get_tick_count()
{
	return rtc_get_tick_count();
}

void reset_tick_count(int enable)
{
	rtc_reset_tick_count(enable);
}

int get_erase_count(void)
{
	struct mmc *mmc;

	mmc = find_mmc_device((int)EMMC_DEV_NUM);

	if (!mmc)
	{
		return -PLR_ENODEV;
	}

	return mmc_get_health(mmc);
}

