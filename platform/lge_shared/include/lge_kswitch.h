/*
 * Copyright (c) 2011-2014, LG Electronics. All rights reserved
 *
 * LGE Kill Switch
 *
 * NOTE:
 * Because kswitch flag should be loaded from ftm item, we must access ftm item to initialize flag.
 * Therefore, kswitch_load_flag() MUST be called at least once between ftm_init() and kswitch_get_flag().
 * target_common_init_ftm() may be a good place to call kswitch_load_flag().
 */

#ifndef __LGE_KSWITCH_H__
#define __LGE_KSWITCH_H__

#include <bits.h>

#define KSWITCH_FLAG_RECOVERY	BIT(0)
#define KSWITCH_FLAG_LAF			BIT(1)

/* kswitch_load_flag()
 * return 0 on failure(it means error, not not-support kswitch or empty value),
 * else return non-0
 */
extern int kswitch_load_flag(void);

/* return 0 on not initialized or invalid flag,
 * else return non-zero
 */
extern int kswitch_is_flag_valid(void);

/* return flag value.
 * you can check each bit on this flag with KSWITCH_FLAG_XXX.
 */
extern int kswitch_get_flag(void);

/* return 0 on false
 * else return no-zero
 * bit: KSWITCH_FLAG_XXX
 */
extern int kswitch_flag_enabled(int bit);
extern int kswitch_flag_disabled(int bit);

#endif	/* __LGE_KSWITCH_H__ */
