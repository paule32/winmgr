#ifndef EVENT_H_
#define EVENT_H_

class Window;

typedef void (*DisplayFuncType)(Window *win);
typedef void (*KeyboardFuncType)(Window *win, int key, bool pressed);
typedef void (*MouseButtonFuncType)(Window *win, int bn, bool pressed, int x, int y);
typedef void (*MouseMotionFuncType)(Window *win, int x, int y);
typedef void (*TimerFuncType)(Window *win);

struct Callbacks {
	DisplayFuncType display;
	KeyboardFuncType keyboard;
	MouseButtonFuncType button;
	MouseMotionFuncType motion;
	TimerFuncType timer;
};

void process_events();

enum TimerMode {TIMER_ONESHOT, TIMER_REPEAT};

void set_window_timer(Window *win, unsigned int msec, TimerMode mode = TIMER_ONESHOT);

#endif
