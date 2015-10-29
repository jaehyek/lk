/*
 * Copyright (c) 2014 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef __LGE_VERIFIED_BOOT_H__
#define __LGE_VERIFIED_BOOT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#include <stdint.h>
#include <stdbool.h>
#endif
#include <boot_verifier.h>

typedef enum {
	SECUREBOOT_LOCKED   = 0,
	VERIFIED_GREEN      = SECUREBOOT_LOCKED,
	SECUREBOOT_VERIFIED = 1,
	VERIFIED_YELLOW     = SECUREBOOT_VERIFIED,
	SECUREBOOT_UNLOCKED = 2,
	VERIFIED_ORANGE     = SECUREBOOT_UNLOCKED,
	SECUREBOOT_TAMPERED = 3,
	VERIFIED_RED        = SECUREBOOT_TAMPERED
} VerifiedBootState;

typedef enum {
	KILLSWITCH_LOCKED = 0,
	KILLSWITCH_UNLOCKED = 1,
} KillSwitchState;

typedef struct killswitch_sig_st
{
	ASN1_INTEGER *version;
	X509_ALGOR *algor;
	ASN1_PRINTABLESTRING *keyalias;
	ASN1_OCTET_STRING *sig;
}KILLSWITCH_SIG;

DECLARE_STACK_OF(KILLSWITCH_SIG)
DECLARE_ASN1_SET_OF(KILLSWITCH_SIG)
DECLARE_ASN1_FUNCTIONS(VERIFIED_BOOT_SIG)

typedef struct lge_keystore_st
{
	ASN1_INTEGER *version;
	ASN1_PRINTABLESTRING *keyalias;
	KEYBAG *mykeybag;
}LGE_KEYSTORE;

DECLARE_STACK_OF(LGE_KEYSTORE)
DECLARE_ASN1_SET_OF(LGE_KEYSTORE)
DECLARE_ASN1_FUNCTIONS(LGE_KEYSTORE)

#ifdef LGE_CHECK_GPT_INTEGRITY
typedef enum {
	GPT_ERROR_NONE = 0,
	GPT_ERROR_INVALID_PARAM,
	GPT_ERROR_NO_PARTITION,
	GPT_ERROR_MISMATCH_PARTITION,
} GptIntegrityState;

typedef struct{
	const char* name;
	const unsigned long long offset;
	const unsigned long long size;
} protected_partition;
#endif // LGE_CHECK_GPT_INTEGRITY

int get_deviceid(char *deviceid);
KillSwitchState   is_killswitch_unlocked();
VerifiedBootState is_bootloader_unlocked();

VerifiedBootState verify_lge_signed_seperatebootimg(unsigned char* boot_hdr,
	unsigned char* kernel_addr, unsigned char* ramdisk_addr, unsigned char* cert_addr);

typedef void (*getKernelHash)(unsigned char* hash, unsigned int size);
VerifiedBootState verify_lge_signed_bootimg(unsigned char* image_addr, uint32_t image_size, unsigned char* keystore_addr, getKernelHash getHash);

#ifdef LGE_CHECK_GPT_INTEGRITY
int check_gpt_integrity_status(const protected_partition *list, const unsigned num_part);
#endif // LGE_CHECK_GPT_INTEGRITY

#ifdef __cplusplus
}
#endif

#endif
