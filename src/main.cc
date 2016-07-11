#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "winnie.h"
#include "client_plugins.h"

static void cleanup();

int main()
{
	if(!winnie_init()) {
		return 1;
	}
	if(!init_client_plugins()) {
		return 1;
	}

	atexit(cleanup);

	Pixmap bg;
	if(bg.load("./data/bg.ppm")) {
		wm->set_background(&bg);
	//} else {
		wm->set_background_color(64, 64, 64);
	}


	while(1) {
		process_events();
	}
}

static void cleanup()
{
	destroy_client_plugins();
	winnie_shutdown();
}
