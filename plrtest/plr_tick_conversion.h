
#ifndef PLR_TICK_CONVERSION_H_
#define PLR_TICK_CONVERSION_H_

#define HARDWARE_EXYNOS
//#define HARDWARE_LG

u32 get_tick2sec(u64 tick);
u32 get_tick2msec(u64 tick);
u32 get_tick2usec(u64 tick);
u32 get_tick2nsec(u64 tick);
u64 get_sec2tick(u32 sec);
u64 get_msec2tick(u32 msec);
u64 get_usec2tick(u32 usec);
u64 get_nsec2tick(u32 nsec);

#endif /* PLR_TICK_CONVERSION_H_ */
