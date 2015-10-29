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

#include <stdint.h>
#include <libfdt.h>
#include <debug.h>
#include <platform.h>

#include <dev_tree.h>
#include <lge_dev_tree.h>
#include <lge_target_dev_tree.h>

#ifdef LGE_WITH_SBL_LOG
#include <lge_sbl_log.h>
#endif

void target_device_tree_update_chosen_node(void *fdt)
{
#ifdef LGE_WITH_SBL_LOG
	uint32_t sbl_time;

	sbl_time = sbl_log_get_delta_time();
	dev_tree_setprop_u32(fdt, "/chosen", "lge,sbl_delta_time", sbl_time);
#endif
	dev_tree_setprop_u64(fdt, "/chosen", "lge,lk_delta_time",
			current_time());
#ifdef LGE_WITH_BOOTLOADER_LOG_SAVE
	dev_tree_setprop_u32(fdt, "/chosen", "lge,log_buffer_phy_addr", LOGBUF_PHYS_ADDR);
	dev_tree_setprop_u32(fdt, "/chosen", "lge,log_buffer_size", LOGBUF_SIZE);
#endif
}

int target_device_tree_update(void *fdt)
{
	int ret = 0;

	/* Add padding to make space for new nodes and properties. */
	ret = fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + DTB_PAD_SIZE);
	if (ret != 0) {
		dprintf(CRITICAL, "Failed to move/resize dtb buffer: %d\n", ret);
		return ret;
	}

	target_device_tree_update_chosen_node(fdt);

	return ret;
}
