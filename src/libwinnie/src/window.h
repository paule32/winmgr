#ifndef WINDOW_H_
#define WINDOW_H_

#include <vector>

#include "geom.h"
#include "event.h"

class Window {
public:
	enum State {STATE_NORMAL, STATE_MINIMIZED, STATE_MAXIMIZED, STATE_SHADED};

private:
	char *title;
	State state;

	Rect rect;
	Rect normal_rect; // normal state rectangle managed by the wm

	Callbacks callbacks;

	std::vector<Window*> children;
	Window* parent;

	bool dirty;
	bool managed; // whether the wm manages (+decorates) this win
	bool focusable;

public:
	Window();
	~Window();

	const Rect &get_rect() const;
	Rect get_absolute_rect() const;
	bool contains_point(int ptr_x, int ptr_y);

	void move(int x, int y);
	void resize(int x, int y);

	void set_title(const char *s);
	const char *get_title() const;

	/* mark this window as dirty, and notify the window manager
	 * to repaint it, and anything it used to cover.
	 */
	void invalidate();

	void draw(Rect *dirty_region);
	void draw_children(const Rect &dirty_region);

	unsigned char *get_win_start_on_fb();
	int get_scanline_width();

	void set_managed(bool managed);
	bool get_managed() const;

	void set_focusable(bool focusable);
	bool get_focusable() const;

	bool get_dirty() const;

	void set_display_callback(DisplayFuncType func);
	void set_keyboard_callback(KeyboardFuncType func);
	void set_mouse_button_callback(MouseButtonFuncType func);
	void set_mouse_motion_callback(MouseMotionFuncType func);
	void set_timer_callback(TimerFuncType func);

	const DisplayFuncType get_display_callback() const;
	const KeyboardFuncType get_keyboard_callback() const;
	const MouseButtonFuncType get_mouse_button_callback() const;
	const MouseMotionFuncType get_mouse_motion_callback() const;
	const TimerFuncType get_timer_callback() const;

	// win hierarchy
	void add_child(Window *win);
	void remove_child(Window *win);

	Window **get_children();
	int get_children_count() const;

	const Window *get_parent() const;
	Window *get_parent();

	void set_state(State state);
	State get_state() const;

	// XXX remove if not needed
	friend class WindowManager;
};

#endif	// WINDOW_H_
