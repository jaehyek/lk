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
 *     * Neither the name of The Linux Fundation, Inc. nor the names of its
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

#ifndef __LGE_BOOTCMD_H
#define __LGE_BOOTCMD_H

#include <list.h>

#define BOOTCMD_MAX_CMDLINE_LEN	        1024
#define BOOTCMD_ARG_KEY_LEN             64
#define BOOTCMD_ARG_VALUE_LEN           64

struct bootcmd_arg {
	struct list_node list;
	char key[BOOTCMD_ARG_KEY_LEN];
	char value[BOOTCMD_ARG_VALUE_LEN];
};

extern int bootcmd_set_value_by_key(const char *key, const char *value);
extern int bootcmd_add_new_key(const char *key);
extern int bootcmd_add_pair(const char *key, const char *value);
extern int bootcmd_list_init(void);
extern void bootcmd_list_show(void);
extern void bootmode_get_reboot_reason_str(char*);
extern char *bootcmd_update_cmdline(const char *cmdline);
extern void bootcmd_list_deinit(char *cmdline);
#endif
