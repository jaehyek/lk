
////////////////////////////////////////////////////////////////////////////////////
// This is 64 bit time tick management module under 32 bit tick system that
// supports time service in precision of second level.
//
// This the algorithm and source code is fully implememted and seriousely tested 
// by Sang Jeong Woo(woosj@elixirflash.com)  in June 2015.
//
// If you have any question or comment about this code, feedback any time 
//  via e-mail digitect38@gmail.com or woosj@elixirflash.com
////////////////////////////////////////////////////////////////////////////////////
// Revisions
// 1. @sj 150620, Initial
// 2. @sj 150710, Second-level-Precision System timer driven wrap around logic introduced.
// 3. @sj 150717, The unstability that belong to the tick uncertainty of sub-second-level timer is removed.
// 4. @sj 150721, Remove the potential of recursive call

#include "plr_type.h"
#include "plr_common.h"
#include "plr_deviceio.h"
#include "plr_err_handling.h"
#include "plr_hooking.h"

#include "plr_rtc64.h"


#define SIGN(a) ((a) > 0 ? 1 : -1)
#define ABS(a)  ((a)*SIGN(a))


//////////////////////////////////////////////////////////////////////////////////////
// 2. Internal constant and variables
//////////////////////////////////////////////////////////////////////////////////////


const u32	TICK_BITS = 32;						// You may change the value as the one of system supports

static u32	_tick_base;							// The value used as the tick origin (start value)
static u32	_tick_offset;						// The value caught while system booting

static u32	_sec_base;							// The value calculated from _tick_base
static u32	_sec_offset;						// The value caught from system

static s64	_tick_up;							// Keeps upper 32 bits of 64bit tick value

////////////////////////////////////////////////////////////////////////////////////

typedef enum {
	RTC_ENABLE,
	RTC_DISABLE
} RTC_ACCESS;

//////////////////////////////////////////////////////////////////////////////////////
// 3. Implementations
//////////////////////////////////////////////////////////////////////////////////////

static u32 _rtc64_get_time(void)
{
	return get_rtc_time() - _sec_offset + _sec_base;	
}

void rtc64_set_tick_count(u32 tick_base)
{
	s64 t1 = -1LL;

	
	reset_tick_count(RTC_DISABLE);
	reset_tick_count(RTC_ENABLE);	
	
	

	_tick_up = 0LL;        	// used for upper 32 bit 

	if (t1 < 0LL) { 		// If the values are upside down, resample it
		_tick_offset	= get_tick_count();
		_sec_offset 	= get_rtc_time();
			
		_tick_base		= tick_base;
		_sec_base		= (s64)get_tick2sec(_tick_base);
	}

}

void rtc64_reset_tick_count(void)
{
	rtc64_set_tick_count(0);
}


// Gets time tick count in 64 bit resolution. 

s64 rtc64_get_tick_count(void)
{
	s64 t64 = 0;	// result
	s64 tsec;		// tick converted from second since 1970

	u32 t32;		// 32 bit tick
	u32 s1;			// compensated raw second


	t32 = get_tick_count() + _tick_base - _tick_offset;
	s1 = _rtc64_get_time();
		
	while (1)	{

		t64 = (_tick_up << TICK_BITS) | (s64)t32;
		
		tsec = (s64)get_sec2tick(s1);
	
		if (tsec < (t64 + (1UL << 31))) break;
		_tick_up++;
	}

	return t64;
}


////////////////////////////////////////////////////////////////////////////////////
//
// RTC64 Unit Tests
//
////////////////////////////////////////////////////////////////////////////////////


#if RTC64_UNIT_TEST == TRUE

static void test_rtc_64__tick_increment()
{
	int i;
	s64 t1, t2;
	u32 msec = 10;

	rtc64_set_tick_count(~0UL - (u32)TICK_RES_NS_INV);	// Let current tick 1 sect before the maximum of 32 bit tick.

	t1 = rtc64_get_tick_count();

	plr_info("tick_value(increment): ");

	for (i=0; i < 3000;i++) {
		mdelay(msec);
		t2 = rtc64_get_tick_count();
		plr_info("%lld(%lld) ", t2, t2 - t1);
		
		SJD_ASSERT(t2 - t1 > (s64)get_msec2tick(msec), "Fail!!!, Tick value is decreased !");
		t1 = t2;
	};

}

static void test_rtc_64__tick_sync()
{
	int i;
	u32 s1, s2;
	s64 t1, t2;
	u32 msec = 10;

	for (i = 0; i < 3000; i++) {
		t1 = (s64)rtc64_get_tick_count();
		s1 = _rtc64_get_time();
		t2 = (s64)get_sec2tick(s1);
		s2 = (s64)get_tick2sec(t1);		
		plr_info("(s1, s2), (t1, t2) = (%u, %u), (%lld, %lld)\n", s1, s2, t1, t2);		
		
		SJD_ASSERT( ABS(t2 - t1) < TICK_RES_NS_INV, "Fail!!!, Tick is not synchronized with second!");	// if tick difference is larger than 1 sec	
	}
}

void test_rtc_64(void)
{
	
	rtc64_reset_tick_count();

	test_rtc_64__tick_sync();
	test_rtc_64__tick_increment();
}


#endif

