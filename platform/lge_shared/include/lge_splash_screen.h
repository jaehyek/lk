#ifndef __LGE_SPLASH_SCREEN_H
#define __LGE_SPLASH_SCREEN_H


#include <stdint.h>
#include <dev/fbcon.h>


void splash_drawglyph(uint8_t *pixels, uint32_t paint, uint32_t stride,
                uint32_t *glyph);

void splash_scroll_up(void);


void splash_clear_line(int line);

void splash_clear_color(unsigned int color, unsigned int fg);

int splash_puts(const char *str);

void display_lge_splash_screen(const char *image_name);

void display_lge_splash_screen_from_emmc(const char *image_name);

void display_string(char *out, int x, int y, int font_width, int font_height);

void display_bar(char * parrVal, unsigned val_len, unsigned bar_height, int start_x, int start_y);

void display_lge_image_on_screen(void);

void display_lge_splash_screen_from_emmc(const char *image_name);

#endif
