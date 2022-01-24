#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <cairo.h>
#include <cairo-xlib.h>

typedef struct win {
	Display* dpy;
	int scr;

	Window win;
	GC gc;

	cairo_surface_t* surface;
	
	int width, height;
	KeyCode quit_code;
} win_t;

//X11 window utilities
static void win_init(win_t* win);
static void win_loop(win_t* win, void (*redraw)(cairo_t*,win_t*));
static void win_deinit(win_t* win);

//Cairo surface utilities
static cairo_t* create_surface(win_t* win);

static void win_init(win_t* win)
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

static void win_deinit(win_t* win)
{
	XDestroyWindow(win->dpy, win->win);
	XCloseDisplay(win->dpy);
}

static cairo_t* create_surface(win_t* win)
{
	Visual* visual = DefaultVisual(win->dpy, DefaultScreen(win->dpy));
	//XClearWindow(win->dpy, win->win);
	win->surface = cairo_xlib_surface_create(
		win->dpy, win->win, visual,
		win->width, win->height);
	return cairo_create(win->surface);
}

static void win_loop(win_t* win, void (*redraw)(cairo_t*,win_t*))
{
	XEvent event;
	cairo_t* root;

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
				
				root = create_surface(win);
			} break;
			case Expose:
			{
				redraw(root, win);
			} break;
		}
	}
}

static void lorenz_step(int i, double* ax, double* ay, double* az)
{
	double rho = 28.0;
	double sigma = 10.0;
	double beta = 8.0 / 3.0;
	double delta = 0.001;	

	double x = ax[i];
	double y = ay[i];
	double z = az[i];

	double dx = sigma *(y - x);
	double dy = x * (rho - z) - y;
	double dz = x*y - beta * z;

	ax[i+1] = x + dx * delta;
	ay[i+1] = y + dy * delta;
	az[i+1] = z + dz * delta;
}

// Redraws the full image (after a resize, for example)
static void redraw(cairo_t* root, win_t* win)
{
	int size = 50000;
	double ax[size];
	double ay[size];
	double az[size];
	// Step 0
	ax[0] = 1.0;
	ay[0] = 0.5;
	az[0] = 0.5;

	// Make a grid
	double centerX = win->width / 2.0;
	double centerY = win->height / 2.0;
	double deltaX = win->width / 100.0;
	double deltaY = win->height / 100.0;
	cairo_set_line_width(root, 0.5);
	cairo_move_to(root, centerX + ax[0] * deltaX, centerY + ay[0] + deltaY);
	cairo_set_source_rgba(root, 0.5, 0.2, 0.3, 1);
	

	// Draw the attractor
	for(int i = 0; i < size - 1; i++)
	{
		lorenz_step(i, ax, ay, az);
		cairo_line_to(root, centerX + ax[i] * deltaX, centerY + ay[i] * deltaY);
	}

	cairo_stroke(root);
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


	// Initialize root pointers
	win_init(&win);

	win_loop(&win, redraw);

	win_deinit(&win);

	printf("Hello World!\n");
	return 0;
}
