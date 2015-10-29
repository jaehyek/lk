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
 
#include "plr_type.h"

static unsigned int state[16];

static uint index;

void well512_seed(unsigned int nSeed)
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

unsigned int well512_rand(void)
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
