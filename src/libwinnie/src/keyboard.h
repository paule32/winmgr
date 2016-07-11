#ifndef KEYBOARD_H_
#define KEYBOARD_H_

bool init_keyboard();
void destroy_keyboard();

bool client_open_keyboard(void *smem_start, int offset);
void client_close_keyboard();

int get_keyboard_fd();
void process_keyboard_event();

#endif	// KEYBOARD_H_
