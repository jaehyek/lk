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

#include <lge_smem.h>
#include <debug.h>

extern int get_qem(void);

int smem_get_vendor0_info(smem_vendor0_type *info)
{
#ifdef LGE_WITH_SMEM_VENDOR
	if (info)
		return smem_read_alloc_entry(SMEM_ID_VENDOR0, info, sizeof(smem_vendor0_type));
#endif

	return -1;
}

int smem_get_vendor1_info(smem_vendor1_type *info)
{
#ifdef LGE_WITH_SMEM_VENDOR
	if (info)
		return smem_read_alloc_entry(SMEM_ID_VENDOR1, info, sizeof(smem_vendor1_type));
#endif

	return -1;
}

int smem_get_vendor2_info(smem_vendor2_type *info)
{
#ifdef LGE_WITH_SMEM_VENDOR
	if (info)
		return smem_read_alloc_entry(SMEM_ID_VENDOR2, info, sizeof(smem_vendor2_type));
#endif

	return -1;
}

int smem_get_pon_status_info(smem_power_on_status_type *info)
{
#ifdef LGE_WITH_SMEM_PON_STATUS
	if (info) {
		return smem_read_alloc_entry(SMEM_POWER_ON_STATUS_INFO,
				info, sizeof(smem_power_on_status_type));
	}
#endif

	return -1;
}

void smem_set_vendor1_info(unsigned int cable_type)
{
#ifdef LGE_WITH_SMEM_VENDOR
	smem_vendor1_type lge_hw_smem_id1_info;
	smem_vendor2_type lge_hw_smem_id2_info;
	unsigned success;

	lge_hw_smem_id1_info.cable_type = cable_type;
	lge_hw_smem_id1_info.qem = get_qem();

#if TARGET_BUILD_USER
	lge_hw_smem_id2_info.build_info = BUILD_TARGET_USER;
#elif TARGET_BUILD_USERDEBUG
	lge_hw_smem_id2_info.build_info = BUILD_TARGET_USERDEBUG;
#elif TARGET_BUILD_ENG
	lge_hw_smem_id2_info.build_info = BUILD_TARGET_ENG;
#endif

	success = smem_write_alloc_entry(SMEM_ID_VENDOR1, &lge_hw_smem_id1_info, sizeof(smem_vendor1_type));
	if (!success)
		dprintf(CRITICAL, "ERROR: unable to write shared memory for ID_VENDOR1\n");

	success = smem_write_alloc_entry(SMEM_ID_VENDOR2, &lge_hw_smem_id2_info, sizeof(smem_vendor2_type));
	if (!success)
		dprintf(CRITICAL, "ERROR: unable to write shared memory for ID_VENDOR2\n");
#endif

	return;
}
#ifdef LGE_PM_BATT_ID
int smem_get_battery_info(smem_batt_info_type *info)
{
	if (info) {
		return smem_read_alloc_entry(SMEM_BATT_INFO, info, sizeof(smem_batt_info_type));
	}

	return -1;
}

int smem_set_battery_info(smem_batt_info_type *info)
{
	if (info) {
		return smem_write_alloc_entry(SMEM_BATT_INFO, info, sizeof(smem_batt_info_type));
	}

	return -1;
}
#endif
