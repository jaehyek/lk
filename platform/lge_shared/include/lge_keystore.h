#ifndef _LGE_KEYSTORE_H
#define _LGE_KEYSTORE_H

extern const unsigned char SECUREBOOT_KEYSTORE[];

#ifdef LGE_BOOTLOADER_UNLOCK
extern const unsigned char BLUNLOCK_KEYSTORE[];
#endif // LGE_BOOTLOADER_UNLOCK

#ifdef LGE_KILLSWITCH_UNLOCK
extern const unsigned char KSWITCH_KEYSTORE[];
#endif // LGE_KILLSWITCH_UNLOCK

#endif // _LGE_KEYSTORE_H