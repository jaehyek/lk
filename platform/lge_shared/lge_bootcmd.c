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

#include <err.h>
#include <malloc.h>
#include <string.h>
#include <printf.h>
#include <debug.h>
#include <lge_bootcmd.h>

static struct list_node bootcmd_arg_list;
static int bootcmd_initialized;

static const char * const bootcmd_init_keys[] = {
	"lge.uart",
	"lge.bootchart",
	"lge.rev",
#ifdef LGE_PM_BATT_ID
	"lge.battid",
#endif
	"lge.bootreason",
	"maxcpus",
	"boot_cpus",
#ifdef WITH_LGE_KSWITCH
	"kswitch",
#endif
	"androidboot.mode",
#ifdef LGUA_FEATURE_ENABLE
    "androidboot.fota",
    "androidboot.fota_reboot",
#endif
};

static inline int bootcmd_check_key_len(const char *str)
{
	if (strlen(str) >= BOOTCMD_ARG_KEY_LEN) {
		dprintf(ERROR, "%s : %s overflow length\n", __func__, str);
		return 1;
	} else {
		return 0;
	}
}

static inline int bootcmd_check_value_len(const char *str)
{
	if (strlen(str) >= BOOTCMD_ARG_VALUE_LEN) {
		dprintf(ERROR, "%s : %s overflow length\n", __func__, str);
		return 1;
	} else {
		return 0;
	}
}

int bootcmd_list_init(void)
{
	int ret = NO_ERROR;
	unsigned i;
	unsigned num = sizeof(bootcmd_init_keys) / sizeof(char *);

	list_initialize(&bootcmd_arg_list);
	bootcmd_initialized = 1;

	for (i = 0; i < num; i++) {
		ret = bootcmd_add_new_key(bootcmd_init_keys[i]);
		if (ret < NO_ERROR) {
			dprintf(ERROR, "%s: initalized fail at the %d\n", __func__, i);
			break;
		}
	}

	return ret;
}

static struct bootcmd_arg *bootcmd_get_by_key(const char *key)
{
	struct bootcmd_arg *arg;

	if (bootcmd_check_key_len(key))
		return NULL;

	list_for_every_entry(&bootcmd_arg_list, arg, struct bootcmd_arg, list) {
		if (!strncmp(arg->key, key, BOOTCMD_ARG_KEY_LEN))
			return arg;
	}

	return NULL;
}

static int bootcmd_set_value(struct bootcmd_arg *arg, const char *value)
{
	int value_len = strlen(value);

	if (bootcmd_check_value_len(value))
		return ERR_INVALID_ARGS;

	memset(arg->value, 0, strlen(arg->value));
	strncpy(arg->value, value, value_len);

	return NO_ERROR;
}

int bootcmd_set_value_by_key(const char *key, const char *value)
{
	struct bootcmd_arg *arg;

	if (!key || !value) {
		dprintf(CRITICAL, "%s: invalid key=%p or value=%p\n", __func__, key, value);
		return ERR_INVALID_ARGS;
	}

	if (bootcmd_check_key_len(key) || bootcmd_check_value_len(value))
		return ERR_INVALID_ARGS;

	arg = bootcmd_get_by_key(key);
	if (arg == NULL) {
		dprintf(CRITICAL, "%s: bootcmd list not found, key=%s\n", __func__, key);
		return ERR_NOT_FOUND;
	}

	return bootcmd_set_value(arg, value);
}

int bootcmd_add_new_key(const char *key)
{
	struct bootcmd_arg *arg;

	if (!key) {
		dprintf(CRITICAL, "%s: invalid key\n", __func__);
		return ERR_INVALID_ARGS;
	}

	if (bootcmd_check_key_len(key)) {
		dprintf(CRITICAL, "%s: invalid key size\n", __func__);
		return ERR_NOT_VALID;
	}

	arg = bootcmd_get_by_key(key);
	if (arg != NULL) {
		dprintf(CRITICAL,
			"%s: already exist key(%s)\n", __func__, arg->key);
		return ERR_ALREADY_EXISTS;
	}

	arg = malloc((sizeof(struct bootcmd_arg) + 4) & (~3));
	if (!arg) {
		dprintf(CRITICAL, "%s: memory allocation failed\n", __func__);
		return ERR_NO_MEMORY;
	}
	memset(arg, 0, sizeof(struct bootcmd_arg));

	strncpy(arg->key, key, strlen(key));
	list_add_tail(&bootcmd_arg_list, &arg->list);

	return NO_ERROR;
}

int bootcmd_add_pair(const char *key, const char *value)
{
	struct bootcmd_arg *arg;

	if (bootcmd_check_key_len(key) || bootcmd_check_value_len(value))
		return ERR_INVALID_ARGS;

	arg = bootcmd_get_by_key(key);

	if (arg != NULL) {
		if (!strncmp(arg->value, value, BOOTCMD_ARG_VALUE_LEN)) {
			dprintf(CRITICAL, "%s: already exist key & value(%s, %s)\n",
				__func__, arg->key, arg->value);
			return ERR_ALREADY_EXISTS;
		} else {
			dprintf(INFO, "%s: value changed from %s to %s at %s\n",
					__func__, arg->value, value, arg->key);
			bootcmd_set_value(arg, value);
		}
	} else  {
		bootcmd_add_new_key(key);
		bootcmd_set_value_by_key(key, value);
	}

	return NO_ERROR;
}

void bootcmd_list_show(void)
{
	struct bootcmd_arg *arg;
	int i = 0;

	if (!bootcmd_initialized) {
		dprintf(ERROR, "%s: bootcmd is not initialized\n", __func__);
		return;
	}

	dprintf(INFO, "\nappended LGE boot command list is belows:\n");
	list_for_every_entry(&bootcmd_arg_list, arg, struct bootcmd_arg, list) {
		dprintf(INFO, " [%2d] key=%s, value=%s\n", i, arg->key, arg->value);
		i++;
	}

	dprintf(INFO, "\n");
}

char *bootcmd_update_cmdline(const char *cmdline)
{
	char *final_cmdline;
	struct bootcmd_arg *arg;
	char *dst;
	const char *src;
	int cmdline_len;

	if (!bootcmd_initialized) {
		dprintf(CRITICAL,
			"%s: you should call 'bootcmd_init() before\n",
			__func__);
		return NULL;
	}

	cmdline_len = strlen(cmdline) + 1;
	list_for_every_entry(&bootcmd_arg_list, arg, struct bootcmd_arg, list) {
		cmdline_len += 1 + strlen(arg->key);
		if (arg->value[0])
			cmdline_len += 1 + strlen(arg->value);
	}

	if (cmdline_len >= BOOTCMD_MAX_CMDLINE_LEN) {
		dprintf(CRITICAL,
			"%s: too much long cmdline size(%d)\n", __func__,
			cmdline_len);
		return NULL;
	}

	final_cmdline = malloc((cmdline_len + 4) & (~3));
	if (!final_cmdline) {
		dprintf(CRITICAL, "%s: memory allocation failed\n", __func__);
		return NULL;
	}
	memset(final_cmdline, 0, cmdline_len);
	dst = final_cmdline;
	src = cmdline;

	while ((*dst++ = *src++))
		;

	list_for_every_entry(&bootcmd_arg_list, arg, struct bootcmd_arg, list) {
		if (arg->value[0])
			dprintf(INFO, "bootcmd: %s=%s\n", arg->key, arg->value);
		else
			dprintf(INFO, "bootcmd: %s\n", arg->key);

		src = arg->key;
		--dst;
		*dst++ = ' ';
		while ((*dst++ = *src++))
			;

		if (arg->value[0]) {
			src = arg->value;
			--dst;
			*dst++ = '=';
			while ((*dst++ = *src++))
				;
		}
	}

	return final_cmdline;
}

void bootcmd_list_deinit(char *cmdline)
{
	struct bootcmd_arg *arg;

	if (!bootcmd_initialized) {
		return;
	}

	while ((arg = list_remove_head_type(&bootcmd_arg_list, struct bootcmd_arg, list))) {
		free(arg);
		arg = NULL;
	}

	if (cmdline) {
		free(cmdline);
		cmdline = NULL;
	}

	bootcmd_initialized = 0;
	dprintf(INFO, "%s : remove all bootcmd_arg\n", __func__);
}
