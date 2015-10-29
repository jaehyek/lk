/*
 * Copyright (c) 2012, LG Electronics. All rights reserved.
 * LGE splash screen images
 */

#ifndef __LGE_SPLASH_FHD_H
#define __LGE_SPLASH_FHD_H

#include "bootlogo_image_FHD.h"

struct image_info {
	const char *name;
	int width;
	int height;
	int pos_x;
	int pos_y;
	char *image;
};

struct image_info lg_logo_image = {
	.name   = "lg_logo_image",
	.width  = 1080,
	.height = 1920,
	.pos_x  = 0,
	.pos_y  = 0,
	.image  = lglogo_image,
};

struct image_info unlock_lg_logo_image = {
	.name   = "unlock_lg_logo_image",
	.width  = 128,
	.height = 128,
	.pos_x  = 950,
	.pos_y  = 10,
	.image  = unlock_lglogo_image_rle,
};

#ifdef LGE_WITH_QUICKCOVER_IMAGE
struct image_info quick_lglogo_image = {
	.name   = "quick_lglogo_image",
	.width  = 572,
	.height = 284,
	.pos_x  = 257,
	.pos_y  = 376,
	.image  = quick_lglogo_image_rle,
};

struct image_info quick_powered_android_image = {
	.name   = "quick_powered_android_image",
	.width  = 339,
	.height = 98,
	.pos_x  = 407,
	.pos_y  = 663,
	.image  = quick_powered_android_image_rle,
};
#endif

struct image_info *splash_images[] = {
	&lg_logo_image,
	&unlock_lg_logo_image,
#ifdef LGE_WITH_QUICKCOVER_IMAGE
	&quick_lglogo_image,
	&quick_powered_android_image,
#endif
};

#endif
