#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <lge_kswitch.h>
#include <lge_ftm.h>

#ifdef WITH_LGE_KSWITCH

#define ENABLED		1
#define DISABLED	0

/* DISABLE_ALL_BIT_ON_EXCEPTION:
 * On exceptional case, kswitch flag will not have valid bits.
 * In this case, kswitch related operation can be unpredictable.
 *
 * If this feature is disabled, all bits are zeroed on exceptional case to enable all.
 * This feature is not official, just a guard for worst exceptional case.
 * So you may not meet this exceptional case.
 */
#define DISABLE_ALL_BIT_ON_EXCEPTION	DISABLED

/* used only for logging */
#define KSWITCH_LOG_TAG	"[KSwitch] "
/* ftm item value format: kswitch=# */
static const char KSWITCH_MANIFEST_STRING[] = {'k', 's', 'w', 'i', 't', 'c', 'h', '=', '\0'};
/* Current implementation uses only serveral bits for flag.
 * If all bits are marked as 1, it means that flag is not initialized.
 */
# define KSWITCH_FLAG_INITIALIZER	~0x0
/* kill switch flag
 * NOTE: bit 1 means disable, bit 0 means enable
 */
static int s_kswitch_flag = KSWITCH_FLAG_INITIALIZER;

int kswitch_load_flag()
{
	char item[LGFTM_KILL_SWITCH_SIZE] = {0, };

	/* set zero for simplifying load_kswich_flag function code */
	s_kswitch_flag = 0;

	if (ftm_get_item(LGFTM_KILL_SWITCH, &item) < 0)
	{
		dprintf(INFO, KSWITCH_LOG_TAG "(%s): has no ftm kill switch item\n", __func__);
		/* may be has no kswitch flag case */
		return s_kswitch_flag;
	}
	dprintf(INFO, KSWITCH_LOG_TAG "ftm kill switch item value=%s\n", item);

	if (strncmp(item, KSWITCH_MANIFEST_STRING, sizeof(KSWITCH_MANIFEST_STRING) - 1))
	{
		dprintf(INFO, KSWITCH_LOG_TAG "(%s): ftm kill switch item has invalid format\n", __func__);
		/* may be has no kswitch flag case */
		return s_kswitch_flag;
	}

	/* It is just debugging message for checking memory/stack corruption,
	 * and avoiding WBT complain ^^; */
	if (&(item[sizeof(KSWITCH_MANIFEST_STRING) -1]) == NULL)
	{
		dprintf(CRITICAL, KSWITCH_LOG_TAG "*** (%s): You can not see this message."
													"If you can, check mem/stack corruption. ***\n", __func__);
#if (DISABLE_ALL_BIT_ON_EXCEPTION == ENABLED)
		/* system is unstable */
		s_kswitch_flag = KSWITCH_FLAG_INITIALIZER;
#endif

		return s_kswitch_flag;
	}

	s_kswitch_flag = atoi(&(item[sizeof(KSWITCH_MANIFEST_STRING) - 1]));

	/* Current implementation uses only 2 digit positive number.  */
	if (s_kswitch_flag < 0 || 99 < s_kswitch_flag)
	{
		dprintf(CRITICAL, KSWITCH_LOG_TAG "*** (%s): invalid ftm kill switch range ***\n", __func__);

#if (DISABLE_ALL_BIT_ON_EXCEPTION == ENABLED)
		/* can not trust ftm kswitch flag or unstable system */
		s_kswitch_flag = KSWITCH_FLAG_INITIALIZER;
#endif

		return s_kswitch_flag;
	}

	dprintf(INFO, KSWITCH_LOG_TAG "kill switch flag loading completed!\n");
	dprintf(INFO, "| recovery=%s\n"
								"| laf=%s\n"
								, kswitch_flag_enabled(KSWITCH_FLAG_RECOVERY) ? "enabled":"disabled"
								, kswitch_flag_enabled(KSWITCH_FLAG_LAF) ? "enabled" : "disabled");

	return s_kswitch_flag;
}

int kswitch_is_flag_valid()
{
	return (s_kswitch_flag != KSWITCH_FLAG_INITIALIZER) ? 1 : 0;
}

int kswitch_get_flag()
{
	return s_kswitch_flag;
}

int kswitch_flag_enabled(int bit)
{
	return !(s_kswitch_flag & bit) ? 1 : 0;
}

int kswitch_flag_disabled(int bit)
{
	return (s_kswitch_flag & bit) ? 1 : 0;
}

#else	/* !WITH_LGE_KSWITCH */

/* stub */
int kswitch_load_flag()
{
	return 1;
}

/* stub */
int kswitch_get_flag()
{
	return 0;
}

/* stub */
int kswitch_is_flag_valid()
{
	return 0;
}

/* stub */
int kswitch_flag_enabled(int bit)
{
	return 1;
}

/* stub */
int kswitch_flag_disabled(int bit)
{
	return 0;
}

#endif	/* WITH_LGE_KSWITCH */
