/* Copyright (c) 2013-2015, LGE Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *  * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *   * Neither the name of The Linux Foundation, Inc. nor the names of its
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

#include <debug.h>
#include <arch/arm.h>
#include <platform/timer.h>
#include <malloc.h>

#include <lge_splash_screen.h>
#include <lge_splash.h>

#include <target.h>
#include <partition_parser.h>

#include <string.h>
#if defined(TARGET_USES_CHARGERLOGO)
#include <pm8x41.h>
#include <dev/keys.h>
#include <lge_bootmode.h>
#endif
#if defined(LGE_WITH_QUICKCOVER_IMAGE)
#include <platform/gpio.h>
#endif

#include <scm.h>

#if defined(LGE_WITH_LOAD_IMAGE_FROM_EMMC)
#define BOOT_IMAGE_MAGIC "BOOT_IMAGE_RLE"
#define RGB888_BG_CHG           0x323232
#define RGB888_BLACK            0x000000
#define BLOCK_SZ 2048
#define RAW_RESOURCES_MAGIC_INFO_OFFSET 0
#define RAW_RESOURCES_IMAGE_INFO_OFFSET BLOCK_SZ
#define RAW_RESOURCES_IMAGE_DATA_OFFSET 0
#define RAW_RESOURCES_IMAGE_DATA_OFFSET_V1 (2*BLOCK_SZ)

typedef struct {
	char name[40];
	uint32_t offset;
	uint32_t size;
	uint32_t x_size;
	uint32_t y_size;
	uint32_t x_pos;
	uint32_t y_pos;
} image_info_type;

typedef struct {
	char magic[16];
	uint32_t total_image_num;
	uint32_t version;
} magic_info_type;

extern uint32_t mmc_get_device_blocksize();
static uint32_t raw_resources_version = 0x1001;
#define read_raw_resources(o, b, s) read_emmc("raw_resources", o, b, s)
#endif

#if defined(TARGET_USES_CHARGERLOGO)
int is_boot_lglogo;
#endif

#ifndef NULL
#define NULL    0
#endif

struct rgb_data {
	char r;
	char g;
	char b;
};

#ifdef WITH_LGE_FBCON_ADJUST_FONT
static uint32_t     BGCOLOR;
static uint32_t     FGCOLOR;
#else
static uint16_t     BGCOLOR;
static uint16_t     FGCOLOR;
#endif

int hallic_get_status(void)
{
	int state = 1;
#if defined(LGE_WITH_QUICKCOVER_IMAGE)
#define HALL_IC_GPIO_INIT	46
	gpio_tlmm_config(HALL_IC_GPIO_INIT, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA, 0);
	mdelay(1);
	state = gpio_get(HALL_IC_GPIO_INIT);
	if (state == 0)
		dprintf(INFO, "%s : hall ic enabled\n", __func__);
	else
		dprintf(INFO, "%s : hall ic disabled\n", __func__);
#endif
	return state;
}

static void splash_flush(void)
{
	struct fbcon_config *config = fbcon_display();

	if (config->update_start)
		config->update_start();
	if (config->update_done)
		while (!config->update_done());
}

#ifdef WITH_LGE_FBCON_ADJUST_FONT
void splash_drawglyph(uint8_t *pixels, uint32_t paint, unsigned stride,
		unsigned *glyph)
{
	struct rgb_data *rgbp;
	struct rgb_data rgb = {
		(paint & 0x0000FF),
		(paint & 0x00FF00) >> 8,
		(paint & 0xFF0000) >> 16,
	};
	unsigned x, y, data;

	struct fbcon_config *config = fbcon_display();

	data = glyph[0];
	for (y = 0; y < (FONT_HEIGHT / 2); ++y) {
		for (x = 0; x < FONT_WIDTH; ++x) {
			if (data & 1) {
				rgbp = (struct rgb_data *) pixels;
				rgbp[0]                   = rgb;
				rgbp[1]                   = rgb;
				rgbp[config->width]     = rgb;
				rgbp[config->width + 1] = rgb;
			}
			data >>= 1;
			pixels += 6;
			x++;
		}
		pixels += stride * 6 - FONT_WIDTH * 3;
		y++;
	}

	data = glyph[1];
	for (y = 0; y < (FONT_HEIGHT / 2); ++y) {
		for (x = 0; x < FONT_WIDTH; ++x) {
			if (data & 1) {
				rgbp = (struct rgb_data *) pixels;
				rgbp[0]                   = rgb;
				rgbp[1]                   = rgb;
				rgbp[config->width]     = rgb;
				rgbp[config->width + 1] = rgb;
			}
			data >>= 1;
			pixels += 6;
			x++;
		}
		pixels += stride * 6 - FONT_WIDTH * 3;
		y++;
	}
}
#else
#define FONT_WIDTH	5
#define FONT_HEIGHT	12
void splash_drawglyph(uint8_t *pixels, uint32_t paint, uint32_t stride,
		uint32_t *glyph)
{
	struct rgb_data *rgbp;
	struct rgb_data rgb = {
		(paint & 0x0000FF),
		(paint & 0x00FF00) >> 8,
		(paint & 0xFF0000) >> 16,
	};

	uint32_t x, y, data;
	stride -= FONT_WIDTH;
	data = glyph[0];
	for (y = 0; y < (FONT_HEIGHT / 2); ++y) {
		for (x = 0; x < FONT_WIDTH; ++x) {
			if (data & 1) {
				rgbp = (struct rgb_data *) pixels;
				rgbp[0]                   = rgb;
			}
			data >>= 1;
			pixels += 3;
		}
		pixels += stride*3;
	}

	data = glyph[1];
	for (y = 0; y < (FONT_HEIGHT / 2); ++y) {
		for (x = 0; x < FONT_WIDTH; ++x) {
			if (data & 1) {
				rgbp = (struct rgb_data *) pixels;
				rgbp[0]                   = rgb;
			}
			data >>= 1;
			pixels += 3;
		}
		pixels += stride*3;
	}
}
#endif

void splash_scroll_up(void)
{
	struct fbcon_config *config = fbcon_display();
	int line = fbcon_get_max_posy();

	unsigned char *dst = config->base;
	unsigned char *src = dst + (config->width * FONT_HEIGHT * 3);
	uint32_t count = config->width * (config->height - FONT_HEIGHT) * 3;

	while (count--) {
		*dst++ = *src++;
	}
	splash_clear_line(line);

	splash_flush();
}

void splash_clear_line(int line)
{
	struct fbcon_config *config = fbcon_display();
	uint32_t count = config->width * FONT_HEIGHT;
	unsigned char *dst = config->base + (line * count * config->bpp / 8);
	uint32_t BGCOLOR = fbcon_get_bgcolor();

	struct rgb_data *rgbp;
	struct rgb_data rgb = {
		(BGCOLOR & 0x0000FF),
		(BGCOLOR & 0x00FF00) >> 8,
		(BGCOLOR & 0xFF0000) >> 16,
	};

	while (count--) {
		rgbp  = (struct rgb_data *)dst;
		*rgbp = rgb;
		dst += 3;
	}
	fbcon_set_cur_pos(0, line);
}

static void splash_set_colors(unsigned bg, unsigned fg)
{
	BGCOLOR = bg;
	FGCOLOR = fg;
}

void splash_clear_color(unsigned int color, unsigned int fg)
{
	int i;
	int max_yline = fbcon_get_max_posy();

	splash_set_colors(color, fg);

	for (i = 0; i <= max_yline + 1; i++)
		splash_clear_line(i);

	fbcon_set_cur_pos(0, 0);
}

int splash_puts(const char *str)
{
	while (*str != 0) {
		fbcon_putc(*str);
		_dputc(*str++);
	}
	return 0;
}

void lge_fbcon_clear_color(unsigned int color, unsigned int fg, int start_y, int end_y)
{
	int i;

	BGCOLOR = color;
	FGCOLOR = fg;

	int max_line_y = fbcon_get_max_posy();

	start_y = start_y / FONT_HEIGHT;
	end_y = end_y / FONT_HEIGHT;

	if (end_y > max_line_y)
		end_y = max_line_y;

	for (i = start_y; i <= end_y + 1; i++)
		splash_clear_line(i);

	fbcon_set_cur_pos(0, 0);
	splash_flush();
}

void fbcon_display_font(int type, int num, int x, int y, int font_width, int font_height)
{
	int wcount = 0;
	int max = font_width * font_height;
	struct fbcon_config *config = fbcon_display();
	int stride = config->width - font_width;
	char *font_image = NULL;

	if (type == NUMBER)
		font_image = (char *)font_image_num[num];
	else if (type == ALPHA)
		font_image = (char *)font_image_alpha[num];
	else if (type == ROOTING)
		font_image = (char *)font_image_rooting[num];
	else
		return;

	struct rgb_data *dst = (struct rgb_data *)config->base
		+ y * config->width + x;

	while (max > 0) {
		int n = *font_image++;
		struct rgb_data rgb = {
			font_image[0],
			font_image[1],
			font_image[2],
		};
		if (n > max)
			n = max;
		max -= n;
		while (n--) {
			*dst++ = rgb;
			if (++wcount == font_width) {
				dst += stride;
				wcount = 0;
			}
		}
		font_image += 3;
	}
}

#if defined(LGE_WITH_LOAD_IMAGE_FROM_EMMC)
static int read_emmc(const char *ptn_name, uint32_t offset, void *out_buf, unsigned size)
{
	int mmc_block_size = mmc_get_device_blocksize();
	char *buf = NULL;
	unsigned buf_size = 0;

	if (ptn_name == NULL || size == 0 || out_buf == NULL) {
		dprintf(CRITICAL, "%s: Invalid argument\n", __func__);
		return -1;
	}

	if (target_is_emmc_boot()) {
		int index;
		unsigned long long ptn;
		unsigned long long ptn_size;

		index = partition_get_index(ptn_name);
		if (index == INVALID_PTN) {
			dprintf(CRITICAL, "%s: No '%s' partition found\n", __func__, ptn_name);
			return -2;
		}

		ptn = partition_get_offset(index);
		ptn_size = partition_get_size(index);

		if (ptn_size < offset + size) {
			dprintf(CRITICAL, "%s: Read request out of '%s' boundaries\n", __func__, ptn_name);
			return -3;
		}

		buf_size = ((size + mmc_block_size - 1) / mmc_block_size) * mmc_block_size;
		if (buf_size != size) {
			buf = (char*)malloc(buf_size);
			if (buf == NULL) {
				dprintf(CRITICAL, "%s: memory allocation failed.\n", __func__);
				return -4;
			}
			if (mmc_read(ptn + offset, (unsigned int *)buf, buf_size)) {
				free(buf);
				dprintf(CRITICAL, "%s: Reading MMC failed\n", __func__);
				return -5;
			}
			memcpy(out_buf, buf, size);
			free(buf);
		} else {
			if (mmc_read(ptn + offset, (unsigned int *)out_buf, buf_size)) {
				dprintf(CRITICAL, "%s: Reading MMC failed\n", __func__);
				return -5;
			}
		}
	} else {
		dprintf(CRITICAL, "%s: %s partition not supported for NAND targets.\n", __func__, ptn_name);
		return -6;
	}

	return 0;
}

int read_image_info_from_emmc(image_info_type** img_info)
{
	int retval = -8;
	magic_info_type magic_info;
	int total_image_num;

	if ((retval = read_raw_resources(RAW_RESOURCES_MAGIC_INFO_OFFSET,
					&magic_info, sizeof(magic_info_type)))) {
		dprintf(CRITICAL,"%s: Reading magic info failed\n", __func__);
		return retval;
	}

	if (strncmp((const char*)magic_info.magic,
				BOOT_IMAGE_MAGIC, sizeof(BOOT_IMAGE_MAGIC)) == 0) {
		raw_resources_version = magic_info.version;
		total_image_num = magic_info.total_image_num;
		dprintf(SPEW,"%s: total_image_num (%d)\n", __func__, total_image_num);
		if (total_image_num <= 0) {
			dprintf(CRITICAL,"%s: Reading magic info failed\n", __func__);
			return -9;
		}

		if (*img_info == NULL)
			*img_info = (image_info_type*)malloc(sizeof(image_info_type) * total_image_num);

		if (*img_info == NULL) {
			dprintf(CRITICAL,"%s: Memory allocation for img_info failed.\n", __func__);
			return -10;
		}

		if ((retval = read_raw_resources(RAW_RESOURCES_IMAGE_INFO_OFFSET,
						*img_info, sizeof(image_info_type) * total_image_num))) {
			dprintf(CRITICAL,"%s: Reading image info failed\n", __func__);
			return retval;
		}
		retval = total_image_num;
	} else {
		dprintf(CRITICAL,"%s: No image data\n", __func__);
	}

	return retval;
}

int read_image_data_from_emmc(char *image, int offset, int size)
{
	int start_offset = RAW_RESOURCES_IMAGE_DATA_OFFSET;
	int retval = 0;

	if (raw_resources_version == 0x1001)
		start_offset = RAW_RESOURCES_IMAGE_DATA_OFFSET_V1;

	if ((retval = read_raw_resources(start_offset + offset, image, size))) {
		dprintf(CRITICAL,"%s: Reading image data failed: offset = 0x%X, size = %d\n",
				__func__, offset, size);
		return retval;
	}

	return retval;
}

static int load_888rle_image_from_emmc(int offset, int size, int w, int h, int x, int y)
{
	struct fbcon_config *config = fbcon_display();
	char *image = NULL;
	char image_array[BLOCK_SZ];
	int working_data = 0;
	int wcount = 0;
	unsigned max = w * h;
	unsigned int stride = config->width - w;
	int i;
	int retval = 0;

	struct rgb_data *dst = (struct rgb_data *)config->base
		+ y * config->width + x;
	addr_t fb_start = (addr_t)dst;

	for (i = 0; i < size; i += BLOCK_SZ) {
		working_data = (size >= i + BLOCK_SZ) ? BLOCK_SZ : (size-i);
		if ((retval = read_image_data_from_emmc(image_array, offset + i, working_data)))
			return retval;
		image = image_array;
		while (max > 0 && working_data > 0) {
			unsigned n = *image++;
			if (n == 0)
				return -7;
			struct rgb_data rgb =  {
				image[0],
				image[1],
				image[2],
			};
			if (n > max)
				n = max;
			max -= n;
			while (n--) {
				*dst++ = rgb;
				if (++wcount == w) {
					dst += stride;
					wcount = 0;
				}
			}
			image += 3;
			working_data -= 4;
		}
	}
	arch_clean_invalidate_cache_range((addr_t)fb_start, (addr_t)dst - fb_start);
	splash_flush();

	return 0;
}

int get_image_info(image_info_type *image_info, const char *image_name)
{
	int i, total_num = 0;
	image_info_type *img_info = NULL;
	int ret = -1;

	total_num = read_image_info_from_emmc(&img_info);
	if (total_num > 0) {
		dprintf(SPEW, "%s: Finding image %s\n", __func__, image_name);
		for (i = 0; i < total_num; i++) {
			dprintf(SPEW, "%s: img_info[%d].name %s\n", __func__, i, img_info[i].name);
			if (!strcmp(img_info[i].name, image_name)) {
				memcpy(image_info, &img_info[i], sizeof(image_info_type));
				ret = 0;
				break;
			}
		}
	} else {
		dprintf(CRITICAL, "%s: total_num is invalid (%d)\n", __func__, total_num);
	}

	if (ret)
		dprintf(CRITICAL, "%s: Image not found\n", __func__);

	if(img_info)
		free(img_info);

	return ret;
}

void display_lge_splash_screen_from_emmc(const char *image_name)
{
	image_info_type image_info;
	int retval;
	if (get_image_info(&image_info, image_name) < 0)
		return;

	if ((retval = load_888rle_image_from_emmc(image_info.offset,
					image_info.size,
					image_info.x_size,
					image_info.y_size,
					image_info.x_pos,
					image_info.y_pos))) {
		set_fbcon_display(1);
		dprintf(CRITICAL,"%s: failed: %s, %d\n", __func__, image_name, retval);
		set_fbcon_display(0);
	}
}

#else
static void splash_load_888rle_image(struct image_info *info)
{
	int w = info->width;
	int h = info->height;
	int x = info->pos_x;
	int y = info->pos_y;
	struct fbcon_config *config = fbcon_display();

	int wcount = 0;
	int max = w * h;
	int stride = config->width - w;

	struct rgb_data *dst = (struct rgb_data *)config->base
		+ y * config->width + x;
	char *image = info->image;

	if (max <= 0 || max > config->width * config->height)
		return;

	while (max > 0) {
		uint32_t n = *image++;
		struct rgb_data rgb = {
			image[0],
			image[1],
			image[2],
		};
		if (n > max)
			n = max;
		max -= n;
		while (n--) {
			*dst++ = rgb;
			if (++wcount == w) {
				dst += stride;
				wcount = 0;
			}
		}
		image += 3;
	}
	splash_flush();
}
#endif // LGE_WITH_LOAD_IMAGE_FROM_EMMC

void display_lge_splash_screen(const char *image_name)
{
#if defined(LGE_WITH_LOAD_IMAGE_FROM_EMMC)

#if defined(LGE_WHITE_BOOTLOGO_BG)
	if (!strcmp(image_name, "lglogo_image")) {
		fbcon_clear_color(0x00FFFFFF, 0x00000000);
	}
#endif

	display_lge_splash_screen_from_emmc(image_name);
#else
    struct image_info *ii = NULL;
    int i, max;

    max = sizeof(splash_images) / sizeof(splash_images[0]);
    for (i = 0; i < max; i++) {
        if (!strcmp(splash_images[i]->name, image_name)) {
            ii = splash_images[i];
            break;
        }
    }

    if (!ii)
        return;

    splash_load_888rle_image(ii);
#endif
}

void display_bar(char * parrVal, unsigned val_len, unsigned bar_height, int start_x, int start_y)
{
	struct fbcon_config *config = fbcon_display();
	unsigned ni,nj;
	unsigned nCode128_len, nCnt;
	unsigned nHeight = config->width;
	/* unsigned nWidth  = config->height; */
	unsigned nBytes_per_bpp = ((config->bpp) / 8);
	unsigned image_base = start_y * (config->width);
	unsigned nCheckSum = 0;

	ni = nj = nCnt = 0;
	nCode128_len = BAR_CODE_128_LEN;

	/* for (ni = 0; ni < BAR_CODE_BACKGROUND_HEIGHT; ni++) */
	/* { memset (config->base + ((image_base + (ni * nHeight)) * nBytes_per_bpp), 0xff, nWidth * nBytes_per_bpp);} */

	for (ni = 0; ni < bar_height; ni++) {
		memcpy((config->base) + ((image_base+start_x + (ni * nHeight)) * nBytes_per_bpp),
				BAR_VALUE[BAR_CODE_128B_START_INDEX], nCode128_len*nBytes_per_bpp);
	}

	for (ni = 0; ni < bar_height; ni++) {
		for(nj=1; nj<= val_len; nj++) {
			memcpy((config->base) + ((image_base+start_x + ((nCode128_len)*nj) + (ni * nHeight)) * nBytes_per_bpp),
					BAR_VALUE[(((int)*(parrVal+nj-1))-48)+16], nCode128_len*nBytes_per_bpp);
		}
	}

	nCheckSum = BAR_CODE_128B_START_INDEX;

	for (ni = 1; ni <= val_len; ni++) {
		nCheckSum += ni*((((int)*(parrVal+ni-1))-48)+16);
	}

	nCheckSum %= BAR_CODE_128_CHECKSUM_VAL;

	for (ni = 0; ni < bar_height; ni++) {
		memcpy ((config->base) + ((image_base+start_x+ ((nCode128_len)*(val_len+1)) + (ni * nHeight)) * nBytes_per_bpp),
				BAR_VALUE[nCheckSum], (nCode128_len)*nBytes_per_bpp);
	}

	for (ni = 0; ni < bar_height; ni++) {
		memcpy ((config->base) + ((image_base+start_x+ ((nCode128_len)*(val_len+2)) + (ni * nHeight)) * nBytes_per_bpp),
				BAR_VALUE_END, (nCode128_len+6)*nBytes_per_bpp);
	}

	splash_flush();
	mdelay(100);
}

void display_string(char *out, int x, int y, int font_width, int font_height)
{
	int num;
	unsigned int size = 0;
	int gap = 0;

	if (!out)
		return;

	while (size < 512) {
		if (*out == '\0')
			break;
		if (*out >= 46 && *out <= 58) {         /* number */
			num = *out - 46;                    /* 0~9, : */
			fbcon_display_font(NUMBER, num, x, y, font_width, font_height);
		} else if (*out >= 65 && *out <= 90) {  /* large alpha */
			num = *out - 65;
			fbcon_display_font(ALPHA, num, x, y, font_width, font_height);
		} else if (*out >= 97 && *out <= 122) { /* small alpha */
			num = *out - 97;
			fbcon_display_font(ALPHA, num, x, y, font_width, font_height);
		} else if (*out == 32) {                /* space */
			x -= 15;
		} else
			return;

		x += 30 + gap;
		out++;
		size++;
	}

	splash_flush();
}

/**
 * @ display_lge_image_on_screen
 * this function is for display lge's specific screen image on device
 * Make the rule to meet a requiremen of project
 * 1. boot status Cold or Warm
 * 2. PON reason
 * 3. Key combination rule
 * 4. Connected cable type
 * 5. Wireless charger enabled or not
 * 6. Hall IC enabled or not
 * 7. TBD (etc)
 */
void display_lge_image_on_screen(void)
{
#if defined(TARGET_USES_CHARGERLOGO)
	uint8_t pon_reason = pm8x41_get_pon_reason();
	uint8_t is_cold_boot = pm8x41_get_is_cold_boot();
	usb_cable_type cable_type = bootmode_get_cable();

	if (is_cold_boot && (!(pon_reason & HARD_RST))
		&& (!(pon_reason & KPDPWR_N)) &&
		((pon_reason & USB_CHG) || (pon_reason & DC_CHG) ||
		 (pon_reason & PON1)) &&
		(keys_get_state(KEY_VOLUMEUP) == 0) &&
		(keys_get_state(KEY_VOLUMEDOWN) == 0) &&
		(cable_type != LT_CABLE_56K && cable_type != LT_CABLE_130K &&
		 cable_type != LT_CABLE_910K && cable_type != NO_INIT_CABLE)) {
		is_boot_lglogo = 0;
		display_lge_splash_screen("load_charger_image");
		dprintf(INFO, "[Display] load_charger_image\n");
	} else {
		is_boot_lglogo = 1;
		if (hallic_get_status() == 0) {
			display_lge_splash_screen("quick_lglogo_image");
			dprintf(INFO, "[Display] quick_lglogo_image\n");
		} else {
			display_lge_splash_screen("lglogo_image");
			display_lge_splash_screen("powered_android_image");
//			if (!is_secure_boot_enable())
//				display_lge_splash_screen("secure_unlock_image");
			dprintf(INFO, "[Display] lglogo_image\n");
		}
	}
#else
	if (hallic_get_status() == 0) {
		display_lge_splash_screen("quick_lglogo_image");
		dprintf(INFO, "[Display] quick_lglogo_image\n");
	} else {
		display_lge_splash_screen("lglogo_image");
		display_lge_splash_screen("powered_android_image");
//		if (!is_secure_boot_enable())
//			display_lge_splash_screen("secure_unlock_image");
		dprintf(INFO, "[Display] lglogo_image\n");
	}
#endif
}
