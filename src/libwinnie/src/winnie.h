#ifndef WINNIE_H_
#define WINNIE_H_

#include "event.h"
#include "geom.h"
#include "gfx.h"
#include "keyboard.h"
#include "mouse.h"
#include "text.h"
#include "window.h"
#include "wm.h"

struct Subsys {
	int graphics_offset;
	int keyboard_offset;
	int mouse_offset;
	int text_offset;
	int wm_offset;
};

bool winnie_init();
void winnie_shutdown();

bool winnie_open();
void winnie_close();

long winnie_get_time();

Subsys *get_subsys();

#endif
