#ifndef GFX_H_
#define GFX_H_

#include "geom.h"
#include "pixmap.h"

bool init_gfx();
void destroy_gfx();

bool client_open_gfx(void *smem_start, int offset);
void client_close_gfx();

unsigned char *get_framebuffer();
Pixmap *get_framebuffer_pixmap();

Rect get_screen_size();
int get_color_depth();

void set_clipping_rect(const Rect &clip_rect);
const Rect &get_clipping_rect();

void clear_screen(int r, int g, int b);
void fill_rect(const Rect &rect, int r, int g, int b);

void set_cursor_visibility(bool visible);

void blit(unsigned char *src_img, const Rect &src_rect, unsigned char* dest_img,
		const Rect &dest_rect, int dest_x, int dest_y);

void blit_key(unsigned char *src_img, const Rect &src_rect, unsigned char* dest_img,
		const Rect &dest_rect, int dest_x, int dest_y, int key_r, int key_g, int key_b);

void draw_line(Pixmap *pixmap, int x0, int y0, int x1, int y1, int r, int g, int b);
void draw_polygon(Pixmap *pixmap, int *vpos, int *vtex, int num_verts, int r, int g, int b);

void gfx_update(const Rect &rect);

void wait_vsync(); // vertical synchronization

void get_rgb_order(int *r, int *g, int *b);

#endif //GFX_H_
