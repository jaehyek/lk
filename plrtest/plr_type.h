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
// Note: This code is most readable while TAB size as 4.
//

#ifndef _PLR_TYPE_H_
#define _PLR_TYPE_H_

#define _SIZE_T

typedef unsigned long 		lbaint_t;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

typedef unsigned char		uchar;

typedef unsigned char 		u8;		//@sj 150701
typedef unsigned int 		u32;	//@sj 150701
typedef unsigned long long 	u64;	// joys,2014.11.05
typedef signed char 		s8;		//@sj 150701
typedef signed int			s32;	//@sj 150701
typedef signed long long 	s64;	//@sj 150701

//
// Limits
//

#define PLR_INT_MAX				0x7FFFFFFFL				//@sj 150702
#define PLR_INT_MIN				0x80000000L				//@sj 150702
#define PLR_LONG_MAX			0x7FFFFFFFL				//@sj 150702
#define PLR_LONG_MIN			0x80000000L				//@sj 150702
#define PLR_LONGLONG_MAX		0x7FFFFFFFFFFFFFFFLL	//@sj 150702
#define PLR_LONGLONG_MIN		0x8000000000000000LL	//@sj 150702

#undef FALSE
#define FALSE	0

#undef TRUE
#define TRUE	(!FALSE)

#ifndef NULL
	#define NULL 0
#endif

#endif
