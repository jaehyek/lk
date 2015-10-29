/* Copyright (c) 2013-2014, LGE Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __LGE_SMEM_H
#define __LGE_SMEM_H

#include <smem.h>

/*
 * It should be match to
 *		boot_images/core/lge/src/boot_lge_hw_init.h
 *
 * smem_read_alloc_entry(smem_mem_type_t type, void *buf, int len)
 *		buf MUST be 4byte aligned, and len MUST be a multiple of 8.
 */

#define MODEL_NAME_MAX_LEN      10

/*
 * struct for SMEM_ID_VENDOR0 @ msm_shared/smem.h
 */
typedef struct {
	unsigned int hw_rev;                 /* rev_type */
	char model_name[MODEL_NAME_MAX_LEN]; /* MODEL NAME */
} smem_vendor0_type;

/*
 * struct for SMEM_ID_VENDOR1 @ msm_shared/smem.h
 */
typedef struct {
	uint32_t cable_type;        /* PIF detection */
	uint32_t ddr_end_point;     /* DDR_END_POINT */
	uint32_t port_type;         /* TA/USB */
	uint32_t qem;
} smem_vendor1_type;

/*
 * struct for SMEM_ID_VENDOR2 @ msm_shared/smem.h
 */
typedef struct {
	uint32_t sbl_log_meta_info;
	uint32_t sbl_delta_time;
	uint32_t lcd_maker;
	uint32_t secure_auth;
	uint32_t build_info;
	int32_t modem_reset;
} smem_vendor2_type;

/*
 * struct for SMEM_POWER_ON_STATUS_INFO @ msm_shared/smem.h
 */
typedef struct {
	uint64_t reasons;
} smem_power_on_status_type;

#ifdef LGE_PM_BATT_ID
typedef struct {
	int lge_battery_info;
} smem_batt_info_type;

extern int smem_get_battery_info(smem_batt_info_type *info);
#endif

extern int smem_get_vendor0_info(smem_vendor0_type *info);
extern int smem_get_vendor1_info(smem_vendor1_type *info);
extern int smem_get_vendor2_info(smem_vendor2_type *info);
extern int smem_get_pon_status_info(smem_power_on_status_type *info);
extern void smem_set_vendor1_info(unsigned int cable_type);
#endif
