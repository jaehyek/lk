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
#include "plr_tick_conversion.h"

enum {
	DEPTH_NS = 0,
	DEPTH_US,
	DEPTH_MS,
	DEPTH_S,
	DEPTH_MIN,
	DEPTH_HOUR,
	DEPTH_DAY,
	DEPTH_MON
};

#define MAX_64BIT	(u64)(~0)

//////////////////////////////////////////////////////////////////////////////////////
// 1. Hardware dependant stuffs
//////////////////////////////////////////////////////////////////////////////////////


//1.1 For Exynos system, 1 tick = 30.5 us, 

#ifdef HARDWARE_EXYNOS

const u64 	TICK_RES_NS  		= 30518LL; 	// uset is nano second
const u64 	TICK_RES_NS_INV  	= 32768LL;	// inverse of TICK_RES_NS
////////////////////////////////////////////////////////////////////////////////////

#elif HARDWARE_LG

//1.2 For LG  system, 1 tick = 52 ns, 

const u64	TICK_RES_NS  		= 52LL;		// uset is nano second
const u64	TICK_RES_NS_INV 	= 19200000LL;	// uset is nano second		// 19.2 Mhz
////////////////////////////////////////////////////////////////////////////////////

#else
//1.3 For additional hardware, define here...

// ...

#endif

// kkr 2015-08-12
// depth 값으로 나눌값을 구함
static u64 get_div(int depth)
{
	if (depth < DEPTH_NS)
		depth = DEPTH_NS;
	if (depth > DEPTH_MON)
		depth = DEPTH_MON;

	switch (depth) {
		case DEPTH_NS:
			return 1;
		case DEPTH_US:
			return (u64)1000;
		case DEPTH_MS:
			return (u64)1000*1000;
		case DEPTH_S:
			return (u64)1000*1000*1000;
		case DEPTH_MIN:
			return (u64)1000*1000*1000*60;
		case DEPTH_HOUR:
			return (u64)1000*1000*1000*60*60;
		case DEPTH_DAY:
			return (u64)1000*1000*1000*60*60*24;
		case DEPTH_MON:
			return (u64)1000*1000*1000*60*60*24*30;
			/*x86에서 default case에서 
			 * 64bit 연산이 제대로 수행되지 않는게 발견되
			 * 그냥 default 삭제*/
	}

	return (u64)1000*1000*1000*60*60*24*30;
}


// kkr 2015-08-12
// value와 resolution값으로 time을 구할때 발생할 수있는 overflow를
// 막기 위한 함수
/*
 * depth 0 : return ns
 * depth 1 : return us
 * depth 2 : return ms
 * depth 3 : return s
 * depth 4 : return minutes
 * depth 5 : return hours
 * depth 6 : return days
 * depth 7 : return mounths (30 day)
 * value : 변환할 값
 * depth_state : 변화후 depth값 (주어진 depth보다 높은 depth가 필요할경우
 * 그에 대한 계산을 한후 depth값 반환)
 * */
static u64 value_to_time(u64 value,
		int depth,
		int resolution, int *depth_state)
{
	u64 div;
	
	if (resolution <=0 ){
		printf("resolution is less then or equal 0\n");
		return 0;
	}

	div = get_div(depth);
	if (MAX_64BIT/resolution < value) {
		while ( MAX_64BIT/resolution < value/div){
			if (depth > DEPTH_MON){
				printf("Too big value or too small resolution."
				"Can't calculate value. value %llu, resolution : %d\n"
				,value, resolution);
				*depth_state = -1;
				return 0;
			}
			depth++;
			div = get_div(depth);
		}
		*depth_state = depth;
		return (value/div*resolution + value%div*resolution/div);

	}
	*depth_state = depth;
	return value*resolution/div;
}

//////////////////////////////////////////////////////////////////////////////////
// Helper functions for various time conversion
//////////////////////////////////////////////////////////////////////////////////

u32 get_tick2sec(u64 tick) 
{
	u64 sec = 0;
	int state = 0;

	sec = value_to_time(tick, DEPTH_S, TICK_RES_NS, &state);

	if (state < 0 || state != DEPTH_S)
	{
		return 0;
	}
	
	return (u32)sec;
}

u32 get_tick2msec(u64 tick) 
{
	u64 msec = 0;
	int state = 0;

	msec = value_to_time(tick, DEPTH_MS, TICK_RES_NS, &state);

	if (state < 0 || state != DEPTH_MS)
	{
		return 0;
	}
	
	return (u32)msec;
}

u32 get_tick2usec(u64 tick)
{
	u64 usec = 0;
	int state = 0;

	usec = value_to_time(tick, DEPTH_US, TICK_RES_NS, &state);

	if (state < 0 || state != DEPTH_US)
	{
		return 0;
	}
	
	return (u32)usec;
}

// tick to micro second, has sence for nano sec system
u32 get_tick2nsec(u64 tick)
{
	u64 nsec = 0;
	int state = 0;

	nsec = value_to_time(tick, DEPTH_NS, TICK_RES_NS, &state);

	if (state < 0 || state != DEPTH_NS)
	{
		return 0;
	}
	
	return (u32)nsec;
}

u64 get_sec2tick(u32 sec)
{
	return (u64)sec  * TICK_RES_NS_INV;
}	

u64 get_msec2tick(u32 msec)
{
	return (u64)msec * TICK_RES_NS_INV / 1000ULL;
}

u64 get_usec2tick(u32 usec)
{
	return (u64)usec * TICK_RES_NS_INV / 1000000ULL;
}

// nano second to tick, has sence for nano sec system
u64 get_nsec2tick(u32 nsec)
{
	return (u64)nsec * TICK_RES_NS_INV / 10000000000ULL;
}

