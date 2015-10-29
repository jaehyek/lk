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

#include <libfdt.h>
#include <err.h>
#include <string.h>
#include <printf.h>
#include <debug.h>
#include <lge_dev_tree.h>

int dev_tree_setprop_u32(void *fdt, char *node, const char *property, uint32_t value)
{
	int ret = 0;
	int offset;

	offset = fdt_path_offset(fdt, node);
	if (offset < 0) {
		dprintf(CRITICAL, "Could not found %s node.\n" , node);
		return offset;
	}

	ret = fdt_setprop_u32(fdt, offset, property, value);
	if (ret < 0) {
		dprintf(CRITICAL, "Could not set %d value to %s property in %s node.\n",
				value, property, node);
		return ret;
	}

	return ret;
}

int dev_tree_setprop_u64(void *fdt, char *node, const char *property, uint64_t value)
{
	int ret = 0;
	int offset;

	offset = fdt_path_offset(fdt, node);
	if (offset < 0) {
		dprintf(CRITICAL, "Could not found %s node.\n" , node);
		return offset;
	}

	ret = fdt_setprop_u64(fdt, offset, property, value);
	if (ret < 0) {
		dprintf(CRITICAL, "Could not set %lld value to %s property in %s node.\n",
				value, property, node);
		return ret;
	}

	return ret;
}

int dev_tree_setprop_string(void *fdt, char *node, const char *property, const char *value)
{
	int ret = 0;
	int offset;

	offset = fdt_path_offset(fdt, node);
	if (offset < 0) {
		dprintf(CRITICAL, "Could not found %s node.\n" , node);
		return ret;
	}

	ret = fdt_setprop_string(fdt, offset, property, value);
	if (ret < 0) {
		dprintf(CRITICAL, "Could not set %s value to %s property in %s node.\n",
				value, property, node);
		return ret;
	}

	return ret;
}
