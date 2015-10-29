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
#include "plr_type.h"

static unsigned int state[16];
static uint index;

void well512_write_seed(unsigned int nSeed)
{
	int i = 0;
	unsigned int s = nSeed;
	index = 0;
		
    for (i = 0; i < 16; i++)
    {
        state[i] = s;
		s += s + 73;
    }
	return;
}

unsigned int well512_write_rand(void)
{
    unsigned int a, b, c, d;
	
    a = state[index];
    c = state[(index + 13) & 15];
    b = a ^ c ^ (a << 16) ^ (c << 15);
    c = state[(index + 9) & 15];
    c ^= (c >> 11);
    a = state[index] = b ^ c;
    d = a ^ ((a << 5) & 0xda442d24U);
    index = (index + 15) & 15;
    a = state[index];
    state[index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);

    return state[index];
}

#define SECTORS_TO_PAGE	8
#define CAL_RANDOM_SECTORS(RAND,MIN,MAX)	(((RAND) % ((((MAX) - (MIN)) / SECTORS_TO_PAGE) + 1)) * SECTORS_TO_PAGE) + MIN
inline static uint random_sector_count_min_max(uint min_sectors, uint max_sectors)
{
	if(min_sectors < SECTORS_TO_PAGE || max_sectors <= SECTORS_TO_PAGE)
	{
		plr_err("[%d][%s] Error, MIN < 8 or MAX <= 8 \n", __LINE__, __func__);
		return SECTORS_TO_PAGE;
	}

	if(min_sectors >= max_sectors)
	{	
		plr_err("[%d][%s] Error, MIN >= MAX \n", __LINE__, __func__);
		return SECTORS_TO_PAGE;
	}
	
	return CAL_RANDOM_SECTORS(well512_write_rand(), min_sectors, max_sectors);
}

// 8 ~ 32 50% 40 ~ 512 50%
static uint random_sector_count1(void) 
{
	uint rand = 0;
	
	rand = well512_write_rand() % 120;

	if ( rand < 61 )
		return (rand / 15) * 8 + 8; 
	else 
		return (rand - 60) * 8 + 40;
}

// 64 ~ 512 
static uint random_sector_count2(void) 
{
	return CAL_RANDOM_SECTORS(well512_write_rand(), 64, 512);
}

// 8 ~ 32 25% 40 ~ 2048 75% 
static uint random_sector_count3(void) 
{
	uint rand = 0;
	
	rand = well512_write_rand() % 336;

	if ( rand < 85 )
		return (rand / 21) * 8 + 8; 
	else 
		return (rand - 84) * 8 + 40;
}

// 64 ~ 1024
static uint random_sector_count4(void) 
{	
	return CAL_RANDOM_SECTORS(well512_write_rand(), 64, 1024);
}

// 8 ~ 512
static uint random_sector_count5(void)
{	
	return CAL_RANDOM_SECTORS(well512_write_rand(), 8, 512);
}

// 8 ~ 1024
static uint random_sector_count6(void) 
{
	return CAL_RANDOM_SECTORS(well512_write_rand(), 8, 1024);
}

// 8 80%, 16 ~ 1024 20%
static uint random_sector_count7(void)
{	
	uint rand = (well512_write_rand() % 35);

	if (rand < 28)
		return 8;
	else
		return (16 << (rand - 28));
}

uint random_sector_count(int num)
{
	uint rand = 0;
	switch (num) {
		case 1:
			rand = random_sector_count1();
			break;
		case 2:
			rand = random_sector_count2();
			break;
		case 3:
			rand = random_sector_count3();
			break;
		case 4:
			rand = random_sector_count4();
			break;
		case 5:
			rand = random_sector_count5();
			break;
		case 6:
			rand = random_sector_count6();
			break;
		case 7:
			rand = random_sector_count7();
			break;
		default:
			break;
	}
	return rand;
}
