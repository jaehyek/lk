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
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <bootimg.h>
#include <openssl/evp.h>
#include <openssl/asn1t.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <lge_verified_boot.h>
#include <lge_keystore.h>
#include <lge_ftm.h>
#include <dev/fbcon.h>
#include <partition_parser.h>

#if defined(WITH_LGE_MT6582) || defined(WITH_LGE_MT6592) || defined(WITH_LGE_MTK)
#define VERIFIED_MTK
#endif

#ifdef WIN32
#include <WinSock2.h>
#else // WIN32
#include <platform.h>
#ifdef VERIFIED_MTK
#include <video.h>
#else
#include <smem.h>
extern void hash_find(unsigned char *addr, unsigned int size, unsigned char *digest, unsigned char auth_alg);
#endif // VERIFIED_MTK
#endif // WIN32

enum {
	SECURITY_ERROR_NONE           = 1000,
	SECURITY_ERROR_BOOTIMG_HDR    = 1001,
	SECURITY_ERROR_KERNEL_ADDR    = 1002,
	SECURITY_ERROR_CERTIFICATE    = 1003,
	SECURITY_ERROR_HASH_TYPE      = 1004,
	SECURITY_ERROR_KEYSTORE       = 1005,
	SECURITY_ERROR_VERIFY_KERNEL  = 1006,
	SECURITY_ERROR_VERIFY_RAMDISK = 1007,
	SECURITY_ERROR_VERIFY_DT      = 1008,
	SECURITY_ERROR_VERIFY_BOOTIMG = 1009,
	SECURITY_ERROR_NO_MEMORY      = 1010,
	SECURITY_ERROR_UNLOCK         = 1011,
	SECURITY_ERROR_HASH           = 1012,
	SECURITY_ERROR_PUBLIC_KEY     = 1013,
	SECURITY_ERROR_UNKNOWN,
	SECURITY_ERROR_MAX,
};

enum {
	CRYPTO_AUTH_ALG_MD5     = 0, // DO NOT USE
	CRYPTO_AUTH_ALG_SHA1    = 1,
	CRYPTO_AUTH_ALG_SHA256
};

enum {
	VERIFY_ERROR_NONE             = 2000,
	VERIFY_ERROR_PARAM            = 2001,
	VERIFY_ERROR_PUBLIC_KEY       = 2002,
	VERIFY_ERROR_HASH             = 2003,
	VERIFY_ERROR_SIG              = 2004,
};

#define SECURITY_VERSION_OLD       0x00000010
#define SECURITY_VERSION_2015      0x00000011
#define SECURITY_MAGIC1            0xA16E62C9
#define SECURITY_MAGIC2            0xD5E7FE61
#define SECURITY_MD_MAX_HASH_SIZE  32 // sha1,sha256 only
#define SECURITY_RSA_MAX_SIGN_SIZE 512 // 1024,2048,4096

typedef struct
{
	uint32_t magic1;
	uint32_t magic2;
	uint32_t version;
	uint32_t reserved;
	uint32_t hash_type;
	uint32_t key_size;
	union {
		struct {
			unsigned char kernel_hash[SECURITY_MD_MAX_HASH_SIZE];
			unsigned char kernel_signature[SECURITY_RSA_MAX_SIGN_SIZE];
			unsigned char ramdisk_hash[SECURITY_MD_MAX_HASH_SIZE];
			unsigned char ramdisk_signature[SECURITY_RSA_MAX_SIGN_SIZE];
			unsigned char dt_hash[SECURITY_MD_MAX_HASH_SIZE];
			unsigned char dt_signature[SECURITY_RSA_MAX_SIGN_SIZE];
		} split;
		struct {
			unsigned char image_signature[SECURITY_MD_MAX_HASH_SIZE * 3 + SECURITY_RSA_MAX_SIGN_SIZE * 3];
		} one;
	} data;
	unsigned char kernelhmac[SECURITY_MD_MAX_HASH_SIZE];
	unsigned char extra[360]; // zero padding for boot certificate
} boot_certificate_data_type;

#ifdef LGE_BOOTLOADER_UNLOCK
#define SECURITY_UNLOCK_MAGIC1     0x8DB7159E
#define SECURITY_UNLOCK_MAGIC2     0x2D7ED36B
#define SECURITY_UNLOCK_VERSION    0x00000001

typedef struct
{
	char device_id[96];
	char imei[32];
} unlock_input_data_type;

typedef struct
{
	uint32_t magic1; // 0x8DB7159E
	uint32_t magic2; // 0x2D7ED36B
	uint32_t version; // 1
	uint32_t hash_type; // sha256
	uint32_t key_size; // rsa256
	unsigned char signature[SECURITY_RSA_MAX_SIGN_SIZE]; // raw signature
	unsigned char extra[492];
} unlock_certificate_data_type;
#endif // LGE_BOOTLOADER_UNLOCK

#ifdef LGE_KILLSWITCH_UNLOCK
#define KILLSWITCH_UNLOCK_MAGIC     0x5455534B
typedef struct
{
	char imei[16];
	uint32_t nonce;
	char device_id[65];
	char extra[43];
} kswitch_input_data_type;

typedef struct
{
	uint32_t magic;
	unsigned char signature[1020]; // KILLSWITCH_SIG
} kswitch_certificate_data_type;
#endif

struct md_info {
	const EVP_MD* md;
	const char* name;
};

typedef struct verifystate_st
{
	const char* name;
	RSA* key;
	struct md_info md;
	unsigned char* image_addr;
	uint32_t image_size;
	unsigned char* signature_addr;
	int ret;
} VERIFY_CTX;

struct image_info {
	unsigned char *addr;
	uint32_t size;
};

typedef struct securitystate_st
{
	int verified_todo;
	int verified_done;

	struct boot_img_hdr *boot_hdr;
	uint32_t offset;
	int page_mask;

	struct image_info kernel;
	struct image_info ramdisk;
	struct image_info second;
	struct image_info dt;
	struct image_info image;

	boot_certificate_data_type *cert;

	getKernelHash getHash;
} SECURITY_CTX;

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))

ASN1_SEQUENCE(KILLSWITCH_SIG) = {
	ASN1_SIMPLE(KILLSWITCH_SIG, version, ASN1_INTEGER),
	ASN1_SIMPLE(KILLSWITCH_SIG, algor, X509_ALGOR),
	ASN1_SIMPLE(KILLSWITCH_SIG, keyalias, ASN1_PRINTABLESTRING),
	ASN1_SIMPLE(KILLSWITCH_SIG, sig, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END(KILLSWITCH_SIG)
IMPLEMENT_ASN1_FUNCTIONS(KILLSWITCH_SIG)

ASN1_SEQUENCE(KEY) = {
	ASN1_SIMPLE(KEY, algorithm_id, X509_ALGOR),
	ASN1_SIMPLE(KEY, key_material, RSAPublicKey)
}ASN1_SEQUENCE_END(KEY)
IMPLEMENT_ASN1_FUNCTIONS(KEY);

ASN1_SEQUENCE(KEYBAG) = {
	ASN1_SIMPLE(KEYBAG, mykey, KEY)
}ASN1_SEQUENCE_END(KEYBAG)
IMPLEMENT_ASN1_FUNCTIONS(KEYBAG)

ASN1_SEQUENCE(LGE_KEYSTORE) = {
	ASN1_SIMPLE(LGE_KEYSTORE, version, ASN1_INTEGER),
	ASN1_SIMPLE(LGE_KEYSTORE, keyalias, ASN1_PRINTABLESTRING),
	ASN1_SIMPLE(LGE_KEYSTORE, mykeybag, KEYBAG)
} ASN1_SEQUENCE_END(LGE_KEYSTORE)
IMPLEMENT_ASN1_FUNCTIONS(LGE_KEYSTORE)

#ifdef VERIFIED_MTK
#define verified_dprintf(level, x...) do { { printf(x); } } while (0)
#else
#define verified_dprintf dprintf
#endif

#define error_display(level, x) do { { dprintf(level, x); fbcon_puts(x); } } while (0)

#define SECURITY_VERIFY_IMAGE(a, b, c, d) \
	if (security_ctx.a.addr) { \
		verify_ctx.name = c; \
		verify_ctx.image_addr = security_ctx.a.addr; \
		verify_ctx.image_size = security_ctx.a.size; \
		verify_ctx.signature_addr = security_ctx.cert->data.b##_signature; \
		if (verify_image(&verify_ctx) == false) { \
			ret = d; \
			goto result; \
		} \
		security_ctx.verified_done++;\
	}

#define SECURITY_PARSING_IMAGE(x, y, z) \
	if (security_ctx.boot_hdr->y##_addr) \
	{ \
		security_ctx.x.addr = image_addr + security_ctx.offset; \
		security_ctx.x.size = ROUND_TO_PAGE(security_ctx.boot_hdr->x##_size, security_ctx.page_mask); \
		if (security_ctx.x.size == 0) { \
			security_ctx.x.addr = NULL; \
		} else { \
			verified_dprintf(INFO, "%s  : 0x%x(0x%x)\n", z, (uint32_t)security_ctx.x.addr, security_ctx.x.size); \
			security_ctx.offset += security_ctx.x.size; \
			security_ctx.verified_todo++;\
		} \
	}

static LGE_KEYSTORE *lge_keystore = NULL;

static uint32_t read_der_message_length(unsigned char* input)
{
	uint32_t len = 0;
	int pos = 0;
	uint8_t len_bytes = 1;

	/* Check if input starts with Sequence id (0X30) */
	if (input[pos] != 0x30)
		return len;
	pos++;

	/* A length of 0xAABBCCDD in DER encoded messages would be sequence of
	following octets 0xAA, 0xBB, 0XCC, 0XDD.

	To read length - read each octet and shift left by 1 octect before
	reading next octet.
	*/
	/* check if short or long length form */
	if (input[pos] & 0x80)
	{
		len_bytes = (input[pos] & ~(0x80));
		pos++;
	}
	while (len_bytes)
	{
		/* Shift len by 1 octet */
		len = len << 8;

		/* Read next octet */
		len = len | input[pos];
		pos++; len_bytes--;
	}

	/* Add number of octets representing sequence id and length  */
	len += pos;

	return len;
}

static void load_keystore(unsigned char* keystore_addr)
{
	LGE_KEYSTORE *ks = NULL;
	uint32_t len = 0;
	unsigned char *input = NULL;

	if (keystore_addr == NULL) {
		input = (unsigned char*)SECUREBOOT_KEYSTORE;
	}
	else {
		input = keystore_addr;
	}

	if (lge_keystore != NULL)
		return;

	len = read_der_message_length(input);
	if (!len)
	{
		verified_dprintf(CRITICAL, "keystore length is invalid.\n");
		return;
	}

	ks = d2i_LGE_KEYSTORE(NULL, (const unsigned char**)&input, len);
	if (ks != NULL)
	{
		lge_keystore = ks;
		verified_dprintf(INFO, "%s is loaded\n", lge_keystore->keyalias->data);
	}
}

static void unload_keystore()
{
	if (lge_keystore != NULL) {
		LGE_KEYSTORE_free(lge_keystore);
		lge_keystore = NULL;
	}
}

#ifdef WIN32
static void dump(void* data, size_t size)
{
#define BYTE_IN_LINE 20
	unsigned long address = (unsigned long)data;
	size_t len = (size>128)?128:size;
	size_t count;
	int bytes = BYTE_IN_LINE;
	int i;
	char line_buf[64];
	char temp[8];

	for (count = 0; count < len; count += BYTE_IN_LINE) {
		sprintf(line_buf, "0x%08lx:", address);
		bytes = ((len - count) > BYTE_IN_LINE) ? BYTE_IN_LINE : (len - count);
		for (i = 0; i < bytes; i++) {
			if ((i % 4) == 0) strcat(line_buf, " ");
			sprintf(temp, "%02x", *(const uint8_t *)(address + i));
			strcat(line_buf, temp);
		}
		strcat(line_buf, "\n");
		dprintf(INFO, line_buf);
		address += BYTE_IN_LINE;
	}
}
#endif // WIN32

static bool verify_image(VERIFY_CTX *ctx)
{
	int key_size = 0;
	unsigned char md_value[EVP_MAX_MD_SIZE] = { 0, };
	uint32_t md_len = 0;
	unsigned char plain_text[EVP_MAX_MD_SIZE] = { 0, };
	int plain_len = 0;
	uint32_t len = 0;
	X509_SIG *sig = NULL;

	if (ctx == NULL ||
		ctx->md.md == NULL) {
		ctx->ret = VERIFY_ERROR_PARAM;
		goto err;
	}

#ifdef LGE_QCT_HW_CRYPTO
	if(strcmp(ctx->md.name, "sha1")==0) {
		md_len = EVP_MD_size(ctx->md.md);
		hash_find(ctx->image_addr, ctx->image_size, md_value, CRYPTO_AUTH_ALG_SHA1);
	}
	else if (strcmp(ctx->md.name, "sha256") == 0) {
		md_len = EVP_MD_size(ctx->md.md);
		hash_find(ctx->image_addr, ctx->image_size, md_value, CRYPTO_AUTH_ALG_SHA256);
	}
	else {
		ctx->ret = VERIFY_ERROR_PARAM;
		goto err;
	}
#elif defined(LGE_NVA_HW_CRYPTO)
#error TODO
#elif defined(LGE_ODIN_HW_CRYPTO)
#define CRYS_HASH_SHA1_mode 0
#define CRYS_HASH_SHA256_mode 2
	if(strcmp(ctx->md.name, "sha1")==0) {
		md_len = EVP_MD_size(ctx->md.md);
		hash_find(ctx->image_addr, ctx->image_size, md_value, CRYS_HASH_SHA1_mode);
	}
	else if (strcmp(ctx->md.name, "sha256") == 0) {
		md_len = EVP_MD_size(ctx->md.md);
		hash_find(ctx->image_addr, ctx->image_size, md_value, CRYS_HASH_SHA256_mode);
	}
	else {
		ctx->ret = VERIFY_ERROR_PARAM;
		goto err;
	}
#else
	if (EVP_Digest(ctx->image_addr, ctx->image_size, md_value, &md_len, ctx->md.md, NULL) != 1) {
		ctx->ret = VERIFY_ERROR_HASH;
		goto err;
	}
#endif

#ifdef WIN32
	dump(ctx->image_addr, ctx->image_size);
#endif // WIN32

	if (ctx->key == NULL) {
		ctx->ret = VERIFY_ERROR_PARAM;
		goto err;
	}

	key_size = RSA_size(ctx->key);
	plain_len = RSA_public_decrypt(key_size, ctx->signature_addr, plain_text, ctx->key, RSA_PKCS1_PADDING);
	if (plain_len == -1) {
		ctx->ret = VERIFY_ERROR_PUBLIC_KEY;
		goto err;
	}

	if (plain_len > (int)md_len)
	{
		len = read_der_message_length(plain_text);
		if (len > 0)
		{
			unsigned char* p = plain_text;
			sig = d2i_X509_SIG(NULL, (const unsigned char **)&p, len);
			if (sig == NULL ||
				sig->digest == NULL) {
				ctx->ret = VERIFY_ERROR_SIG;
				goto err;
			}

			if (sig->digest->length == (int)md_len && memcmp(md_value, sig->digest->data, md_len) == 0)
			{
				ctx->ret = VERIFY_ERROR_NONE;
				verified_dprintf(INFO, "%s verified\n", ctx->name);
				goto err;
			}
		}
	}
	else
	{
		if (plain_len == (int)md_len && memcmp(md_value, plain_text, md_len) == 0)
		{
			ctx->ret = VERIFY_ERROR_NONE;
			verified_dprintf(INFO, "%s verified\n", ctx->name);
			goto err;
		}
	}

	ctx->ret = VERIFY_ERROR_HASH;
	goto err;

err:
	if(sig != NULL) {
		X509_SIG_free(sig);
		sig = NULL;
	}

	if (ctx->ret == VERIFY_ERROR_NONE)
		return true;

	verified_dprintf(CRITICAL, "verify_image : %d\n", ctx->ret);
	return false;
}

static void get_md_info(int hash_type, struct md_info *info)
{
	switch (hash_type)
	{
#ifdef WIN32
		case CRYPTO_AUTH_ALG_MD5:
			info->md = EVP_md5();
			info->name = "md5";
			break;
#endif
		case CRYPTO_AUTH_ALG_SHA1:
			info->md = EVP_sha1();
			info->name = "sha1";
			break;
		case CRYPTO_AUTH_ALG_SHA256:
			info->md = EVP_sha256();
			info->name = "sha256";
			break;
		default:
			info->md = NULL;
			info->name = NULL;
			return;
	}
}

int get_deviceid(char *deviceid)
{
#ifdef WIN32
	strcpy(deviceid, "38A760F2F4F61772A53BC4A56D85D781E47D43B277BEA3FEC1AEF8FF4BBAB75B");
#else
	uint32_t qfprom = 0;
	uint32_t lsb = 0;
	uint32_t msb = 0;

#ifdef VERIFIED_MTK
typedef  unsigned int u32;
extern u32 get_devinfo_with_index(u32 index);
	msb = get_devinfo_with_index(13);
	lsb = get_devinfo_with_index(12);
#else
	switch(board_platform_id())
	{
		case MSM8610:
			qfprom = 0xFC4B81D8;
			break;
		case APQ8064:
			qfprom = 0x007000C0;
			break;
		case MSM8626:
		case MSM8974:
		case MSM8974AC:
			qfprom = 0xFC4B81F0;
			break;
		case MSM8916:
		case MSM8936:
		case MSM8939:
		case MSM8952:
			qfprom = 0x0005C008;
			break;
		case APQ8084:
			qfprom = 0xFC4B81F0;
			break;
		case MSM8994:
		case MSM8992:
			qfprom = 0xFC4BC1F0;
			break;
		default:
			verified_dprintf(CRITICAL, "unknown platform id : %d\n", board_platform_id());
			return -1;
	}

	lsb = *((volatile uint32_t*)(qfprom));
	msb = *((volatile uint32_t*)(qfprom + 4));
#endif

#define DEVICEID_INPUT_SIZE 32
#define DEVICEID_OUTPUT_SIZE 64
	char temp[DEVICEID_INPUT_SIZE + 1];
	unsigned char md_value[EVP_MAX_MD_SIZE] = { 0, };
	char deviceid_hash[DEVICEID_OUTPUT_SIZE + 1] = { 0, };
	uint32_t md_len = 0;
	uint32_t i = 0;

	memset(temp, 0x00, DEVICEID_INPUT_SIZE);
	sprintf(temp, "%08X%08X", lsb, msb);
	EVP_Digest(temp, DEVICEID_INPUT_SIZE, md_value, &md_len, EVP_sha256(), NULL);
	for (i = 0; i < md_len; i++) {
		sprintf(temp, "%02X", md_value[i]);
		strcat(deviceid_hash, temp);
	}

	if (deviceid != NULL)
		memcpy(deviceid, deviceid_hash, DEVICEID_OUTPUT_SIZE);
#endif
	return 0;
}

static int get_imei(char *imei)
{
	int ret = ftm_get_item(LGFTM_IMEI, imei);
	if(ret == 0 && strlen(imei) == 0)
		strcpy(imei, "000000000000000");

	// remove \r or \n
	for (unsigned int i = 0; i < strlen(imei); i++) {
		if (imei[i] == '\r' || imei[i] == '\n') {
			imei[i] = '\0';
			break;
		}
	}
	return ret;
}

static uint32_t get_nonce()
{
#ifdef LGE_KILLSWITCH_UNLOCK
	uint32_t nonce = 0;
	uint32_t erase = 0;
	ftm_get_item(LGFTM_KILL_SWITCH_NONCE, (void *)&nonce);
	ftm_set_item(LGFTM_KILL_SWITCH_NONCE, (void *)&erase);
	return nonce;
#endif // LGE_KILLSWITCH_UNLOCK
	return 0;
}

#if 0
static void wait_boot(int ms, bool dload)
{
#ifdef WIN32
	Sleep(ms);
#else
	thread_sleep(ms);
#endif

	if (dload) {
#ifdef WIN32
		verified_dprintf(CRITICAL, "reboot_device(DLOAD)");
#else
		reboot_device(DLOAD);
#endif
	}
}
#endif

#ifdef LGE_BOOTLOADER_UNLOCK
VerifiedBootState is_bootloader_unlocked()
{
	int ret = SECURITY_ERROR_UNLOCK;

	unlock_input_data_type input;
	unlock_certificate_data_type *output = NULL;
	VERIFY_CTX verify_ctx = { 0, };
	assert(sizeof(unlock_certificate_data_type) <= LGFTM_BLUNLOCK_KEY_SIZE);

	output = (unlock_certificate_data_type*)malloc(LGFTM_BLUNLOCK_KEY_SIZE);
	if (output == NULL) {
		verified_dprintf(CRITICAL, "No memory!\n");
		ret = SECURITY_ERROR_NO_MEMORY;
		goto err;
	}

	ftm_get_item(LGFTM_BLUNLOCK_KEY, (void*)output);
	if (output->magic1 != SECURITY_UNLOCK_MAGIC1 ||
		output->magic2 != SECURITY_UNLOCK_MAGIC2 ||
		output->version != SECURITY_UNLOCK_VERSION) {
		verified_dprintf(ALWAYS, "No unlock key!\n");
		ret = SECURITY_ERROR_UNLOCK;
		goto err;
	}
	verified_dprintf(INFO, "Found unlock key!\n");

	memset(&input, 0x00, sizeof(unlock_input_data_type));
	get_deviceid(input.device_id);
	verified_dprintf(INFO, "DeviceID : %s\n", input.device_id);
	get_imei(input.imei);
	verified_dprintf(INFO, "IMEI : %s\n", input.imei);

	load_keystore((unsigned char*)BLUNLOCK_KEYSTORE);
	if (lge_keystore == NULL) {
		ret = SECURITY_ERROR_KEYSTORE;
		goto err;
	}
	verify_ctx.key = lge_keystore->mykeybag->mykey->key_material;
	verify_ctx.name = "unlock";
	verify_ctx.image_addr = (unsigned char*)&input;
	verify_ctx.image_size = sizeof(unlock_input_data_type);
	verify_ctx.signature_addr = output->signature;
	get_md_info(output->hash_type, &verify_ctx.md);

	verify_image(&verify_ctx);

	switch (verify_ctx.ret) {
		case VERIFY_ERROR_NONE:
			ret = SECURITY_ERROR_NONE;
			break;
		case VERIFY_ERROR_PUBLIC_KEY:
			verified_dprintf(CRITICAL, "public key mismatch\n");
			ret = SECURITY_ERROR_PUBLIC_KEY;
			break;
		case VERIFY_ERROR_HASH:
			verified_dprintf(CRITICAL, "unlock key mismatch\n");
			ret = SECURITY_ERROR_HASH;
			break;
		case VERIFY_ERROR_PARAM:
		default:
			ret = SECURITY_ERROR_UNLOCK;
			break;
	}

err:
	if (output != NULL) {
		free(output);
		output = NULL;
	}

	unload_keystore();

	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();

	if (ret == SECURITY_ERROR_NONE)
		return SECUREBOOT_UNLOCKED;
	return SECUREBOOT_LOCKED;
}
#endif

#ifdef LGE_KILLSWITCH_UNLOCK
KillSwitchState is_killswitch_unlocked()
{
	int ret = SECURITY_ERROR_UNLOCK;

	kswitch_input_data_type input;
	kswitch_certificate_data_type *output = NULL;
	VERIFY_CTX verify_ctx = { 0, };
	KILLSWITCH_SIG *sig = NULL;
	unsigned char* sig_addr = NULL;
	uint32_t sig_len = 0;

	assert(sizeof(kswitch_certificate_data_type) <= LGFTM_KILL_SWITCH_UNLOCK_KEY_SIZE);

	output = (kswitch_certificate_data_type*)malloc(LGFTM_KILL_SWITCH_UNLOCK_KEY_SIZE);
	if (output == NULL) {
		verified_dprintf(CRITICAL, "No memory!\n");
		ret = SECURITY_ERROR_NO_MEMORY;
		goto err;
	}
	memset(output, 0x00, LGFTM_KILL_SWITCH_UNLOCK_KEY_SIZE);

	load_keystore((unsigned char*)KSWITCH_KEYSTORE);
	if (lge_keystore == NULL) {
		ret = SECURITY_ERROR_KEYSTORE;
		goto err;
	}

	// re-use output for update kill switch info.
#ifdef LGE_KILLSWITCH_OUTPUT_FORMAT_2
	sprintf((char*)output, "%s,%d", lge_keystore->keyalias->data, 2);
#else
	sprintf((char*)output, "%s,%d", lge_keystore->keyalias->data, 1);
#endif
	ftm_set_item(LGFTM_KILL_SWITCH_UNLOCK_INFO, output);

	ftm_get_item(LGFTM_KILL_SWITCH_UNLOCK_KEY, (void*)output);
	if (output->magic != KILLSWITCH_UNLOCK_MAGIC) {
		verified_dprintf(ALWAYS, "No unlock key!\n");
		ret = SECURITY_ERROR_UNLOCK;
		goto err;
	}

	verified_dprintf(INFO, "Found unlock key!\n");

	sig_addr = output->signature;
	sig_len = read_der_message_length(sig_addr);
	sig = d2i_KILLSWITCH_SIG(NULL, (const unsigned char **)&sig_addr, sig_len);
	if (sig == NULL)
	{
		verified_dprintf(CRITICAL, "sig is null : %d\n", sig_len);
		ret = SECURITY_ERROR_UNLOCK;
		goto err;
	}

	int nid = OBJ_obj2nid(sig->algor->algorithm);
	int hash_type = CRYPTO_AUTH_ALG_MD5;
	switch (nid) {
	case NID_sha256WithRSAEncryption:
		hash_type = CRYPTO_AUTH_ALG_SHA256;
		break;
	case NID_sha1WithRSAEncryption:
		hash_type = CRYPTO_AUTH_ALG_SHA1;
		break;
	default:
		ret = SECURITY_ERROR_HASH_TYPE;
		goto err;
	}

	memset(&input, 0x00, sizeof(kswitch_input_data_type));
#ifdef LGE_KILLSWITCH_OUTPUT_FORMAT_2
	// don't use device id.
#else
	get_deviceid(input.device_id);
	verified_dprintf(INFO, "DeviceID : %s\n", input.device_id);
#endif
	get_imei(input.imei);
	verified_dprintf(INFO, "IMEI : %s\n", input.imei);
	input.nonce = get_nonce();
	verified_dprintf(INFO, "NONCE : %d\n", input.nonce);

	verify_ctx.key = lge_keystore->mykeybag->mykey->key_material;
	verify_ctx.name = "killswitch";
	verify_ctx.image_addr = (unsigned char*)&input;
	verify_ctx.image_size = sizeof(kswitch_input_data_type);
	verify_ctx.signature_addr = sig->sig->data;
	get_md_info(hash_type, &verify_ctx.md);

	verify_image(&verify_ctx);

	switch (verify_ctx.ret) {
	case VERIFY_ERROR_NONE:
		ret = SECURITY_ERROR_NONE;
		break;
	case VERIFY_ERROR_PUBLIC_KEY:
		verified_dprintf(CRITICAL, "public key mismatch : %s\n", sig->keyalias->data);
		ret = SECURITY_ERROR_PUBLIC_KEY;
		break;
	case VERIFY_ERROR_HASH:
		verified_dprintf(CRITICAL, "unlock key mismatch\n");
		ret = SECURITY_ERROR_HASH;
		break;
	case VERIFY_ERROR_PARAM:
	default:
		ret = SECURITY_ERROR_UNLOCK;
		break;
	}

err:
	if (output != NULL) {
		memcpy(output, (ret == SECURITY_ERROR_NONE)? (void*)"KSOK":(void*)"KSNO", 4);
		ftm_set_item(LGFTM_KILL_SWITCH_UNLOCK_KEY, (void*)output);
		free(output);
		output = NULL;
	}

	if (sig != NULL) {
		KILLSWITCH_SIG_free(sig);
		sig = NULL;
	}

	unload_keystore();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();

	if (ret == SECURITY_ERROR_NONE)
		return KILLSWITCH_UNLOCKED;
	return KILLSWITCH_LOCKED;
}
#endif

static void security_error_display(int error)
{
	char temp[64] = {0,};
	error_display(CRITICAL, "--------------------------------------------\n");
	error_display(CRITICAL, "\n");
	error_display(CRITICAL, " Secure booting Error!\n");

	sprintf(temp, " Error Code : %d\n", error);
	error_display(CRITICAL, temp);

	error_display(CRITICAL, "\n");
	error_display(CRITICAL, "--------------------------------------------\n");
}

static VerifiedBootState get_verifiedboot_state(int result)
{
	VerifiedBootState state = SECUREBOOT_TAMPERED;

	if (result != SECURITY_ERROR_NONE) {
		security_error_display(result);
		state = SECUREBOOT_TAMPERED;
	}
	else{
		if (1) {
			verified_dprintf(INFO, "Boot state is locked\n");
			state = SECUREBOOT_LOCKED;
		}
#if 0
		else {
			verified_dprintf(INFO, "Boot state is Verified\n");
			state = SECUREBOOT_VERIFIED;
		}
#endif
	}

	unload_keystore();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	return state;
}

VerifiedBootState verify_lge_signed_seperatebootimg(unsigned char* boot_hdr,
	unsigned char* kernel_addr, unsigned char* ramdisk_addr, unsigned char* cert_addr)
{
	SECURITY_CTX security_ctx = { 0, };
	VERIFY_CTX verify_ctx = { 0, };

	int ret = SECURITY_ERROR_UNKNOWN;

	security_ctx.boot_hdr = (struct boot_img_hdr *)boot_hdr;
	if ((security_ctx.boot_hdr == NULL) ||
		(strncmp((const char*)security_ctx.boot_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0)) {
		ret = SECURITY_ERROR_BOOTIMG_HDR;
		goto result;
	}

	security_ctx.page_mask = security_ctx.boot_hdr->page_size - 1;
	security_ctx.offset = security_ctx.boot_hdr->page_size;

	security_ctx.kernel.addr = kernel_addr;
	security_ctx.kernel.size = ROUND_TO_PAGE(security_ctx.boot_hdr->kernel_size, security_ctx.page_mask);
	if (security_ctx.kernel.addr != NULL) {
		verified_dprintf(INFO, "kernel  : 0x%x(0x%x)\n", (uint32_t)security_ctx.kernel.addr, security_ctx.kernel.size);
		security_ctx.verified_todo++;
	}

	security_ctx.ramdisk.addr = ramdisk_addr;
	security_ctx.ramdisk.size = ROUND_TO_PAGE(security_ctx.boot_hdr->ramdisk_size, security_ctx.page_mask);
	if (security_ctx.ramdisk.addr != NULL) {
		verified_dprintf(INFO, "ramdisk  : 0x%x(0x%x)\n", (uint32_t)security_ctx.ramdisk.addr, security_ctx.ramdisk.size);
		security_ctx.verified_todo++;
	}

	if (security_ctx.kernel.addr == NULL) {
		ret = SECURITY_ERROR_KERNEL_ADDR;
		goto result;
	}

	security_ctx.cert = (boot_certificate_data_type*)cert_addr;
	if (security_ctx.cert == NULL ||
		security_ctx.cert->magic1 != SECURITY_MAGIC1 ||
		security_ctx.cert->magic2 != SECURITY_MAGIC2) {
		ret = SECURITY_ERROR_CERTIFICATE;
		goto result;
	}

	get_md_info(security_ctx.cert->hash_type, &verify_ctx.md);
	if (verify_ctx.md.md == NULL || verify_ctx.md.name == NULL) {
		ret = SECURITY_ERROR_HASH_TYPE;
		goto result;
	}

	load_keystore(NULL);
	if (lge_keystore == NULL) {
		ret = SECURITY_ERROR_KEYSTORE;
		goto result;
	}

	verify_ctx.key = lge_keystore->mykeybag->mykey->key_material;
	if (security_ctx.cert->version == SECURITY_VERSION_OLD) {
		SECURITY_VERIFY_IMAGE(kernel, split.kernel, "kernel", SECURITY_ERROR_VERIFY_KERNEL)
		SECURITY_VERIFY_IMAGE(ramdisk, split.ramdisk, "ramdisk", SECURITY_ERROR_VERIFY_RAMDISK)
	}
	else {
		verified_dprintf(CRITICAL, "%d version not support\n", security_ctx.cert->version);
		ret = SECURITY_ERROR_CERTIFICATE;
		goto result;
	}

	assert(ret == SECURITY_ERROR_UNKNOWN);
	if (security_ctx.verified_done >= 1 &&
		security_ctx.verified_todo == security_ctx.verified_done) {
		ret = SECURITY_ERROR_NONE;
	}

result:
	return get_verifiedboot_state(ret);
}

VerifiedBootState verify_lge_signed_bootimg(unsigned char* image_addr, uint32_t image_size, unsigned char* keystore_addr, getKernelHash getHash)
{
	SECURITY_CTX security_ctx = { 0, };
	VERIFY_CTX verify_ctx = { 0, };

	int ret = SECURITY_ERROR_UNKNOWN;

	security_ctx.boot_hdr = (struct boot_img_hdr *)image_addr;
	if ((security_ctx.boot_hdr == NULL) ||
		(strncmp((const char*)security_ctx.boot_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0)) {
		ret = SECURITY_ERROR_BOOTIMG_HDR;
		goto result;
	}

	security_ctx.page_mask = security_ctx.boot_hdr->page_size - 1;
	security_ctx.offset = security_ctx.boot_hdr->page_size;

	SECURITY_PARSING_IMAGE(kernel, kernel, "kernel")
	SECURITY_PARSING_IMAGE(ramdisk, ramdisk, "ramdisk")
	SECURITY_PARSING_IMAGE(second, second, "second")
#ifndef VERIFIED_MTK
	SECURITY_PARSING_IMAGE(dt, tags, "dt")
#endif

	if (security_ctx.kernel.addr == NULL) {
		ret = SECURITY_ERROR_KERNEL_ADDR;
		goto result;
	}

	security_ctx.cert = (boot_certificate_data_type*)(image_addr + security_ctx.offset);
	if (security_ctx.cert == NULL ||
		security_ctx.cert->magic1 != SECURITY_MAGIC1 ||
		security_ctx.cert->magic2 != SECURITY_MAGIC2) {
		ret = SECURITY_ERROR_CERTIFICATE;
		goto result;
	}

	if (security_ctx.cert->version == SECURITY_VERSION_2015) {
		security_ctx.image.addr = image_addr;
		security_ctx.image.size =
			security_ctx.boot_hdr->page_size +
			security_ctx.kernel.size +
			security_ctx.ramdisk.size +
			security_ctx.second.size +
			security_ctx.dt.size;
		security_ctx.verified_todo = 1;
	}

	if(getHash != NULL) {
		getHash(security_ctx.cert->kernelhmac, SECURITY_MD_MAX_HASH_SIZE);
	}

	get_md_info(security_ctx.cert->hash_type, &verify_ctx.md);
	if (verify_ctx.md.md == NULL || verify_ctx.md.name == NULL) {
		ret = SECURITY_ERROR_HASH_TYPE;
		goto result;
	}

	load_keystore(keystore_addr);
	if (lge_keystore == NULL) {
		ret = SECURITY_ERROR_KEYSTORE;
		goto result;
	}

	verify_ctx.key = lge_keystore->mykeybag->mykey->key_material;
	if (security_ctx.cert->version == SECURITY_VERSION_OLD) {
		SECURITY_VERIFY_IMAGE(kernel, split.kernel, "kernel", SECURITY_ERROR_VERIFY_KERNEL)
		SECURITY_VERIFY_IMAGE(ramdisk, split.ramdisk, "ramdisk", SECURITY_ERROR_VERIFY_RAMDISK)
		SECURITY_VERIFY_IMAGE(dt, split.dt, "dt", SECURITY_ERROR_VERIFY_DT)
	}
	else if (security_ctx.cert->version == SECURITY_VERSION_2015) {
		SECURITY_VERIFY_IMAGE(image, one.image, "all", SECURITY_ERROR_VERIFY_BOOTIMG)
	}
	else {
		verified_dprintf(CRITICAL, "%d version not support\n", security_ctx.cert->version);
		ret = SECURITY_ERROR_CERTIFICATE;
		goto result;
	}

	assert(ret == SECURITY_ERROR_UNKNOWN);
	if (security_ctx.verified_done >= 1 &&
		security_ctx.verified_todo == security_ctx.verified_done) {
		ret = SECURITY_ERROR_NONE;
	}

result:
	return get_verifiedboot_state(ret);
}

#ifdef LGE_CHECK_GPT_INTEGRITY
int check_gpt_integrity_status(const protected_partition *part_list, const unsigned num_part)
{
	unsigned i = 0;
	int index = INVALID_PTN;
	unsigned long long offset = 0;
	unsigned long long size = 0;

	if ( part_list == NULL || num_part == 0 ) {
		verified_dprintf(CRITICAL, "gpt_integrity : invalid param\n");
		return GPT_ERROR_INVALID_PARAM;
	}

	for ( i=0; i<num_part; i++) {
		index = partition_get_index(part_list[i].name);
		verified_dprintf(INFO, "gpt_integrity : %4d-%s\n", index, part_list[i].name);
		if( index == INVALID_PTN) {
			verified_dprintf(CRITICAL, "gpt_integrity : %s partition not exist\n", part_list[i].name);
			return GPT_ERROR_NO_PARTITION;
		}

		offset = partition_get_offset(index);
		size = partition_get_size(index);

		if ( part_list[i].offset != offset || part_list[i].size != size)
		{
			verified_dprintf(CRITICAL, "gpt_integrity : %s partition mismatch : %llx:%llx, %llx:%llx\n",
					part_list[i].name, part_list[i].offset, offset, part_list[i].size, size);
			return GPT_ERROR_MISMATCH_PARTITION;
		}
	}
	return GPT_ERROR_NONE;
}
#endif // LGE_CHECK_GPT_INTEGRITY
