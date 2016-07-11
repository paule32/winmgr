#ifndef MOUSE_H_
#define MOUSE_H_

struct Rect;

bool init_mouse();
void destroy_mouse();

bool client_open_mouse(void *smem_start, int offset);
void client_close_mouse();

void set_mouse_bounds(const Rect &rect);

int get_mouse_fd();
void process_mouse_event();

void get_pointer_pos(int *x, int *y);
int get_button_state();
int get_button(int bn);

#endif	// MOUSE_H_
