#ifndef PLR_RTC64_H_
#define PLR_RTC64_H_

////////////////////////////////////////////////////////////////////////////////////
// This is 64 bit time tick management module under 32 bit tick system that
// supports time service in precision of second level.
//
// This the algorithm and source code is fully implememted and seriousely tested 
// by Sang Jeong Woo(woosj@elixirflash.com)  in June 2015.
//
// If you have any question or comment about this code, feedback any time 
// via e-mail digitect38@gmail.com or woosj@elixirflash.com
////////////////////////////////////////////////////////////////////////////////////

#include "plr_type.h"

#define RTC64_UNIT_TEST FALSE // Turn on for RTC64 Unit test

void rtc64_reset_tick_count(void); 			// rtc_reset_tick_count 64 bit version with simplified arguments.
s64  rtc64_get_tick_count(void);			// rtc_get_tick_count 64 bit version

void test_rtc_64(void);						// test function of RTC 64

////////////////////////////////////////////////////////////////////////////////////

#endif
