#include <algorithm>
#include <limits.h>
#include <stdexcept>
#include <stdio.h>

#include "gfx.h"
#include "mouse.h"
#include "mouse_cursor.h"
#include "shalloc.h"
#include "text.h"
#include "window.h"
#include "winnie.h"
#include "wm.h"

#define DCLICK_INTERVAL 400

WindowManager *wm;

static void display(Window *win);
static void mouse(Window *win, int bn, bool pressed, int x, int y);
static void motion(Window *win, int x, int y);

bool init_window_manager()
{
	void *wm_mem;
	if(!(wm_mem = sh_malloc(sizeof *wm))) {
		return false;
	}

	wm = new (wm_mem) WindowManager;

	get_subsys()->wm_offset = (int)((char*)wm - (char*)get_pool());

	return true;
}

void destroy_window_manager()
{
	wm->~WindowManager();
	sh_free(wm);
}


bool client_open_wm(void *smem_start, int offset)
{
	wm = (WindowManager*) ((unsigned char*)smem_start + offset);
	return true;
}

void client_close_wm()
{
}

void WindowManager::create_frame(Window *win)
{
	Window *frame = new Window;
	Window *parent = win->get_parent();

	frame->set_display_callback(display);
	frame->set_mouse_button_callback(mouse);
	frame->set_mouse_motion_callback(motion);
	frame->set_focusable(false);
	frame->add_child(win);

	windows.push_back(frame);

	Rect win_rect = win->get_rect();
	frame->move(win_rect.x - frame_thickness,
			win_rect.y - frame_thickness - titlebar_thickness);
	frame->resize(win_rect.width + frame_thickness * 2,
			win_rect.height + frame_thickness * 2 + titlebar_thickness);

	win->move(frame_thickness, frame_thickness + titlebar_thickness);
	parent->add_child(frame);
}

void WindowManager::destroy_frame(Window *win)
{
	Window *frame = win->parent;
	if(!frame) {
		return;
	}

	if(grab_win == win) {
		release_mouse();
	}

	std::list<Window*>::iterator it;
	it = std::find(windows.begin(), windows.end(), frame);
	if(it != windows.end()) {
		root_win->add_child(win);
		windows.erase(it);
		delete frame;
	}
}

WindowManager::WindowManager()
{
	if(!wm) {
		wm = this;
	} else {
		throw std::runtime_error("Trying to create a second instance of WindowManager!\n");
	}

	root_win = new Window;
	root_win->resize(get_screen_size().width, get_screen_size().height);
	root_win->move(0, 0);
	root_win->set_managed(false);

	grab_win = 0;
	focused_win = 0;
	background = 0;

	bg_color[0] = 210;
	bg_color[1] = 106;
	bg_color[2] = 106;

	frame_thickness = 6;
	titlebar_thickness = 18;

	set_focused_frame_color(64, 64, 64);
	set_unfocused_frame_color(160, 160, 160);
	set_bevel_size(2);

	mouse_cursor.set_image(mouse_cursor_width, mouse_cursor_height);
	unsigned char *pixels = mouse_cursor.get_image();

	for(int i=0; i<mouse_cursor_height; i++) {
		for(int j=0; j<mouse_cursor_width; j++) {
			int val = mouse_cursor_bw[i * mouse_cursor_width + j];
			*pixels++ = val;
			*pixels++ = val;
			*pixels++ = val;
			*pixels++ = 255;
		}
	}
}

WindowManager::~WindowManager()
{
	delete root_win;
}

void WindowManager::invalidate_region(const Rect &rect)
{
	dirty_rects.push_back(rect);
}

void WindowManager::process_windows()
{
	if(dirty_rects.empty()) {
		return;
	}

	std::list<Rect>::iterator drit = dirty_rects.begin();
	Rect uni = *drit++;
	while(drit != dirty_rects.end()) {
		uni = rect_union(uni, *drit++);
	}
	dirty_rects.clear();

	wait_vsync();

	if(!background) {
		fill_rect(uni, bg_color[0], bg_color[1], bg_color[2]);
	}
	else {
		blit(background->pixels, Rect(0, 0, background->width, background->height),
				get_framebuffer(), get_screen_size(), 0, 0);
	}

	root_win->draw_children(uni);

	// draw mouse cursor
	int mouse_x, mouse_y;
	get_pointer_pos(&mouse_x, &mouse_y);

	blit_key(mouse_cursor.get_image(), mouse_cursor.get_rect(),
			get_framebuffer(), get_screen_size(), mouse_x, mouse_y,
			0, 0, 0);

	Rect mouse_rect(mouse_x, mouse_y, mouse_cursor.get_width(), mouse_cursor.get_height());
	invalidate_region(mouse_rect);

	gfx_update(uni);
}

void WindowManager::add_window(Window *win)
{
	if(!win || win == root_win) {
		return;
	}

	root_win->add_child(win);

	if(windows.empty()) {
		focused_win = win;
	}

	if(win->get_managed()) {
		create_frame(win);
	}

	windows.push_back(win);
}

void WindowManager::remove_window(Window *win)
{
	std::list<Window*>::iterator it;
	it = std::find(windows.begin(), windows.end(), win);

	if(it != windows.end()) {
		windows.erase(it);
	}
}

void WindowManager::set_focused_window(Window *win)
{
	if(win && win == focused_win) {
		return;
	}

	if(focused_win) {
		// invalidate the frame (if any)
		Window *parent = focused_win->get_parent();
		if(parent && parent != root_win) {
			parent->invalidate();
		}
	}

	if(!win) {
		focused_win = 0;
		return;
	}

	if(win->get_focusable()) {
		focused_win = win;
		return;
	}

	Window **children = win->get_children();
	for(int i=0; i<win->get_children_count(); i++) {
		if(children[0]->get_focusable()) {
			set_focused_window(children[0]);
			return;
		}
	}

	focused_win = 0;
}

const Window *WindowManager::get_focused_window() const
{
	return focused_win;
}

Window *WindowManager::get_focused_window()
{
	return focused_win;
}

Window *WindowManager::get_window_at_pos(int pointer_x, int pointer_y)
{
	Window *root_win = wm->get_root_window();
	Window **children = root_win->get_children();
	for(int i=root_win->get_children_count() - 1; i>=0; i--) {
		if(children[i]->contains_point(pointer_x, pointer_y)) {
			return children[i];
		}
	}

	return 0;
}

Window *WindowManager::get_root_window() const
{
	return root_win;
}

void WindowManager::set_focused_frame_color(int r, int g, int b)
{
	frame_fcolor[0] = r;
	frame_fcolor[1] = g;
	frame_fcolor[2] = b;
}

void WindowManager::get_focused_frame_color(int *r, int *g, int *b) const
{
	*r = frame_fcolor[0];
	*g = frame_fcolor[1];
	*b = frame_fcolor[2];
}

void WindowManager::set_unfocused_frame_color(int r, int g, int b)
{
	frame_ucolor[0] = r;
	frame_ucolor[1] = g;
	frame_ucolor[2] = b;
}

void WindowManager::get_unfocused_frame_color(int *r, int *g, int *b) const
{
	*r = frame_ucolor[0];
	*g = frame_ucolor[1];
	*b = frame_ucolor[2];
}

void WindowManager::set_frame_size(int sz)
{
	frame_thickness = sz;
}

int WindowManager::get_frame_size() const
{
	return frame_thickness;
}

void WindowManager::set_titlebar_size(int sz)
{
	titlebar_thickness = sz;
}

int WindowManager::get_titlebar_size() const
{
	return titlebar_thickness;
}

void WindowManager::set_bevel_size(int sz)
{
	bevel_sz = sz;
}

int WindowManager::get_bevel_size() const
{
	return bevel_sz;
}

void WindowManager::set_background_color(int r, int g, int b)
{
	bg_color[0] = r;
	bg_color[1] = g;
	bg_color[2] = b;
}

void WindowManager::get_background_color(int *r, int *g, int *b) const
{
	*r = bg_color[0];
	*g = bg_color[1];
	*b = bg_color[2];
}

void WindowManager::set_background(const Pixmap *pixmap)
{
	if(background) {
		delete background;
	}

	if(pixmap) {
		background = new Pixmap(*pixmap);
	}
	else {
		background = 0;
	}
}

const Pixmap *WindowManager::get_background() const
{
	return background;
}

Window *WindowManager::get_grab_window() const
{
	return grab_win;
}

void WindowManager::grab_mouse(Window *win)
{
	grab_win = win;
}

void WindowManager::release_mouse()
{
	grab_win = 0;
}

void WindowManager::raise_window(Window *win)
{
	if(!win) {
		return;
	}

	Window *parent = win->get_parent();
	if(parent != root_win) {
		if(parent->get_parent() == root_win) {
			win = parent;
		}
		else {
			return;
		}
	}

	root_win->remove_child(win);
	root_win->add_child(win);
}

void WindowManager::sink_window(Window *win)
{
	if(!win) {
		return;
	}

	std::list<Window*>::iterator it;
	it = std::find(windows.begin(), windows.end(), win);
	if(it != windows.end()) {
		windows.erase(it);
		windows.push_front(win);
	}
}

void WindowManager::maximize_window(Window *win)
{
	win->normal_rect = win->rect;

	Rect rect = get_screen_size();

	Window *frame;
	if((frame = win->get_parent())) {
		frame->normal_rect = frame->rect;
		frame->resize(rect.width, rect.height);
		frame->move(rect.x, rect.y);

		rect.width -= frame_thickness * 2;
		rect.height -= frame_thickness * 2 + titlebar_thickness;
	}
	else {
		win->move(0, 0);
	}

	win->resize(rect.width, rect.height);
	win->set_state(Window::STATE_MAXIMIZED);

	invalidate_region(rect);
}

void WindowManager::unmaximize_window(Window *win)
{
	win->resize(win->normal_rect.width, win->normal_rect.height);
	win->move(win->normal_rect.x, win->normal_rect.y);

	Window *frame;
	if((frame = win->get_parent())) {
		frame->resize(frame->normal_rect.width, frame->normal_rect.height);
		frame->move(frame->normal_rect.x, frame->normal_rect.y);
	}

	win->set_state(Window::STATE_NORMAL);
}

static void display(Window *win)
{
	//frame display:
	Window *child = win->get_children()[0];
	int r, g, b;
	Rect rect = win->get_absolute_rect();

	int tbar = wm->get_titlebar_size();
	int frm = wm->get_frame_size();



	if(child == wm->get_focused_window()) {
		wm->get_focused_frame_color(&r, &g, &b);
	}
	else {
		wm->get_unfocused_frame_color(&r, &g, &b);
	}

	// draw the four frame sides (top, bottom, left, right)
	fill_rect(Rect(rect.x, rect.y, rect.width, frm), r, g, b);
	fill_rect(Rect(rect.x, rect.y + rect.height - frm, rect.width, frm), r, g, b);
	fill_rect(Rect(rect.x, rect.y + frm, frm, rect.height - 2 * frm), r, g, b);
	fill_rect(Rect(rect.x + rect.width - frm, rect.y + frm, frm, rect.height - 2 * frm), r, g, b);
	// draw the titlebar
	fill_rect(Rect(rect.x + frm, rect.y + frm, rect.width - 2 * frm, tbar), r, g, b);


	int val = (r + g + b) / 3;
	int roffs = val < 128 ? r / 2 : (255 - r) / 2;
	int goffs = val < 128 ? g / 2 : (255 - g) / 2;
	int boffs = val < 128 ? b / 2 : (255 - b) / 2;

	// draw bevels
	int dark_r = r - roffs;
	int dark_g = g - goffs;
	int dark_b = b - boffs;

	int lt_r = r + roffs;
	int lt_g = g + goffs;
	int lt_b = b + boffs;


	set_text_position(rect.x + frm + 2, rect.y + frm + tbar - 5);
	set_text_color(80, 80, 80);
	draw_text(child->get_title());
	set_text_position(rect.x + frm + 1, rect.y + frm + tbar - 6);
	set_text_color(255, 255, 255);
	draw_text(child->get_title());

	int bevel = wm->get_bevel_size();
	fill_rect(Rect(rect.x, rect.y, bevel, rect.height), lt_r, lt_g, lt_b);
	fill_rect(Rect(rect.x, rect.y + rect.height - bevel, rect.width, bevel), dark_r, dark_g, dark_b);
	fill_rect(Rect(rect.x + rect.width - bevel, rect.y, bevel, rect.height), dark_r, dark_g, dark_b);
	fill_rect(Rect(rect.x, rect.y, rect.width, bevel), lt_r, lt_g, lt_b);

	Rect inner = Rect(rect.x + frm, rect.y + frm + tbar, rect.width - frm * 2, rect.height - frm * 2 - tbar);
	fill_rect(Rect(inner.x - bevel, inner.y + inner.height, inner.width + 2 * bevel, bevel), lt_r, lt_g, lt_b);
	fill_rect(Rect(inner.x - bevel, inner.y - bevel, bevel, inner.height + 2 * bevel), dark_r, dark_g, dark_b);
	fill_rect(Rect(inner.x + inner.width, inner.y - bevel, bevel, inner.height + 2 * bevel), lt_r, lt_g, lt_b);
	fill_rect(Rect(inner.x - bevel, inner.y - bevel, inner.width + 2 * bevel, bevel), dark_r, dark_g, dark_b);
}

static int prev_x, prev_y;

static void mouse(Window *win, int bn, bool pressed, int x, int y)
{
	static long last_click = 0;

	if(bn == 0) {
		if(pressed) {
			wm->grab_mouse(win);
			wm->raise_window(win);
			prev_x = x;
			prev_y = y;
		}
		else {
			long time = winnie_get_time();
			if((time - last_click) < DCLICK_INTERVAL) {
				Window *child = win->get_children()[0];
				Window::State state = child->get_state();
				if(state == Window::STATE_MAXIMIZED) {
					wm->unmaximize_window(child);
				}
				else if(state == Window::STATE_NORMAL) {
					wm->maximize_window(child);
				}
			}
			last_click = time;

			wm->release_mouse();
		}
	}
}

static void motion(Window *win, int x, int y)
{
	int left_bn = get_button(0);

	if(left_bn) {
		int dx = x - prev_x;
		int dy = y - prev_y;
		prev_x = x - dx;
		prev_y = y - dy;

		if(win->get_children()[0]->get_state() != Window::STATE_MAXIMIZED) {
			Rect rect = win->get_rect();
			win->move(rect.x + dx, rect.y + dy);
		}
	}
}
