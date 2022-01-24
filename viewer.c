#include <stdio.h>
#include <stdlib.h>

#include <cairo.h>
#include <cairo-xlib.h>

typedef struct win {
	Display *dpy;
	int scr;

	Window win;
	GC gc;
	
	int width, height;
	KeyCode quit_code;
} win_t;


static void win_init(win_t *win);

static void win_init(win_t *win)
{
	Window root;

	win->width = 400;
	win->height = 400;
	
	root = DefaultRootWindow(win->dpy);
	win->scr = DefaultScreen(win->dpy);

	win->win = XCreateSimpleWindow(
		win->dpy, root, 0, 0,
		win->width, win->height, 0,
		BlackPixel(win->dpy, win->scr),BlackPixel(win->dpy, win->scr));

	win->quit_code = XKeysymToKeycode(win->dpy, XStringToKeysym("Q"));

	XSelectInput(
		win->dpy, win->win,
		KeyPressMask | StructureNotifyMask | ExposureMask);

	XMapWindow(win->dpy, win->win);
}

static void win_deinit(win_t *win)
{
	XDestroyWindow(win->dpy, win->win);
	XCloseDisplay(win->dpy);
}

static void win_loop(win_t *win)
{
	XEvent event;

	while(true)
	{
		XNextEvent(win->dpy, &event);
		switch(event.type)
		{
			case KeyPress:
			{
				XKeyEvent *kevent = &event.xkey;
				if(kevent->keycode == win->quit_code) return;
			} break;
			case ConfigureNotify:
			{
				XConfigureEvent *cevent = &event.xconfigure;
				
				win->width = cevent->width;
				win->height = cevent->height;
			} break;
			case Expose:
			{
				//TODO: REDRAW THE SCREEN!
			} break;
		}
	}
}

int main()
{
	win_t win;
	
	win.dpy = XOpenDisplay(0);

	if(win.dpy == NULL)
	{
		fprintf(stderr, "Failed to open display\n");
		return 1;
	}


	win_init(&win);

	win_loop(&win);

	win_deinit(&win);

	printf("Hello World!\n");
	return 0;
}
