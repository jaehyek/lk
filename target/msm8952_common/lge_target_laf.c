#include <debug.h>
#include <lge_target_common.h>
#include <lge_bootmode.h>

#ifdef WITH_LGE_DOWNLOAD
extern int get_ftm_webflag(void);
extern void set_ftm_webflag(int status);
extern int bootcmd_add_pair(const char *key, const char *value);
extern int get_qem(void);
extern void lge_pm_check_factory_reset(unsigned *pon_addr, unsigned *poff_addr);

unsigned laf_restart_reason = 0;
int laf_mode = -1;
int boot_into_laf = 0;
#ifdef LGE_PM_CHARGING_CHARGERLOGO
extern unsigned boot_into_chargerlogo;
#endif//LGE_PM_CHARGING_CHARGERLOGO

#define LAF_DLOAD_MODE   				0x6C616664 /* lafd */
#define LAF_RESTART_MODE 				0x6f656d52
#define WDL_STATUS_NORMAL            	0
#define WDL_STATUS_DLOAD_1TIME       	1
#define WDL_STATUS_DLOAD_ALWAYS      	2
#define WDL_STATUS_FORCE_NORMAL_BOOT 	5

int target_laf_check_boot_into_laf(unsigned reboot_mode)
{
#ifdef LGE_PM_FACTORY_CABLE
	unsigned fr_pon_reason = 0x0;
	unsigned fr_poff_reason = 0x0;
#endif

    /* check webdlownload flag */
    switch(get_ftm_webflag())
    {
         case WDL_STATUS_DLOAD_1TIME:
         case WDL_STATUS_DLOAD_ALWAYS:
             dprintf(INFO, "webdload flag set.\n");
             laf_mode = 4;
             goto exit;
             break;

         case WDL_STATUS_FORCE_NORMAL_BOOT:
             laf_mode = 0;
#ifdef LGE_PM_CHARGING_CHARGERLOGO
             boot_into_chargerlogo = 0;
#endif
             goto exit;
             break;
    }

	/* if complete the flashing, add the download complete propperty.
	 * it will be use the setup wizard. do not enter dlownload mode.
	 */
	if (reboot_mode == LAF_RESTART_MODE) {
		dprintf(INFO, "if flashing complete, add the dlcomplete property.\n");
		bootcmd_add_pair("androidboot.dlcomplete", "1");
#ifdef LGE_PM_CHARGING_CHARGERLOGO
		boot_into_chargerlogo = 0;
#endif
		laf_mode = 0;
		goto exit;
	}
	else {
		bootcmd_add_pair("androidboot.dlcomplete", "0");
	}

	if ((reboot_mode & 0xFFFF0000) == LGE_RB_MAGIC) {
		dprintf(INFO, "do not enter dload mode.\n");
		laf_mode = 0;
		goto exit;
	}

	/* pressed the volume up key. */
	if (boot_into_laf) {
		dprintf(INFO, "detect the volume up key + USB.\n");
		laf_mode = 1;
		goto exit;
	}

	/* reboot reason : DLOAD_F command. */
	if (reboot_mode  == LAF_DLOAD_MODE) {
		dprintf(INFO, "reboot reason is the laf boot.\n");
		laf_mode = 2;
		goto exit;
	}

	/* attached the 910K USB Cable. */
	if (bootmode_get_cable() == LT_CABLE_910K) {
        /* not enter laf mode after factory/mode reset */
#ifdef LGE_PM_FACTORY_CABLE
		lge_pm_check_factory_reset(&fr_pon_reason, &fr_poff_reason);
		dprintf(INFO, "910K : fr_pon_reason = 0x%x, fr_poff_reason = 0x%x\n", fr_pon_reason, fr_poff_reason );
#ifdef LGE_P1V
		/* skip laf mode when connecting 910k cable and mode-reset */
		if (fr_pon_reason == 0x21 && fr_poff_reason == 0x2)
#else
		if ((fr_pon_reason == 0x11 || fr_pon_reason == 0x41) 
			&& fr_poff_reason == 0x2)
#endif
#endif //LGE_PM_FACTORY_CABLE
		{
            dprintf(INFO, "not enter laf mode after factory reset or mode reset for factory download.\n" );
            laf_mode = 0;
#ifdef LGE_PM_CHARGING_CHARGERLOGO
			boot_into_chargerlogo = 0;
#endif
			goto exit;
		}
		dprintf(INFO, "detected the 910K USB cable.\n");
		laf_mode = 3;
		goto exit;
	}

	if ((reboot_mode & 0xFFFF0000) == LGE_RB_MAGIC) {
		dprintf(INFO, "do not enter dload mode. after modifying\n");
		laf_mode = 0;
		goto exit;
	}

	laf_mode = 0;

exit:
    if (get_ftm_webflag() != WDL_STATUS_DLOAD_ALWAYS)
        set_ftm_webflag(WDL_STATUS_NORMAL);

	return laf_mode;
}

char *laf_get_bootcmd_usb_cable(void)
{
	switch (bootmode_get_cable())
	{
		case LT_CABLE_56K:
			return "56K";
		case LT_CABLE_130K:
			return "130K";
		case LT_CABLE_910K:
			return "910K";
		default:
	        /* when the qem value and the user usb cable,
	           if it is set, the laf usb property has the force factory cable values. */
	        if(get_qem() == 1)
				return "910K";
#if defined (CARRIER_SPR)
		    return "SUSER";
#elif defined (LGE_ALTEV2_VZW)
		return "VUSER_ALTEV2";
#else
	        return "USER";
#endif
	}
}

#endif
