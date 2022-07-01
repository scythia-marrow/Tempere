// C imports
#include <math.h>

// C++ imports
#include <vector>


// Module imports
#include "palette.h"
#include "brushes.h"
#include "render.h"
#include "constraints.h"
#include "geom.h"

// TODO: make weighted sums a basic utility!

struct dials
{
	double com = -1.0;
	double siz = -1.0;
	double ori = -1.0;
};


void draw_circle(cairo_t* drawer, double area, Vertex mid, struct dials d)
{
	// Constraint satisfaction
	double len = sqrt(area) / 2.0;
	double scale = (len / 2.0) + ((len / 2.0) * d.siz);
	scale = len * 0.5;
	// Draw
	// Translate along the orientation axis and scale a bit!
	cairo_translate(drawer, mid.x, mid.y);
	cairo_rotate(drawer, 2.0 * M_PI * d.ori);
	cairo_scale(drawer, 0.7, 1);
	cairo_arc(drawer, 0.0, 0.0, scale, 0.0, 2.0 * M_PI);
	cairo_fill(drawer);
}

void draw_ngon(cairo_t* drawer, int N, double area, Vertex mid, struct dials d)
{
	// Constraint satisfaction
	double len = sqrt(area) / 2.0;
	double scale = (len / 2.0) + ((len / 2.0) * d.siz);
	// Utility variables
	double delta = 2.0 * M_PI / N;
	bool init = false;
	// Scale to orientation
	cairo_translate(drawer, mid.x, mid.y);
	cairo_rotate(drawer, 2.0 * M_PI * d.ori);
	cairo_scale(drawer, 0.7, 1);
	for(int n = 0; n < N; n++)
	{
		double x_rel = sin(delta * n) * scale;
		double y_rel = cos(delta * n) * scale;
		Vertex v = {x_rel, y_rel};
		if(!init) { cairo_move_to(drawer, v.x, v.y); init = true; }
		else { cairo_line_to(drawer, v.x, v.y); }
	}
	cairo_fill(drawer);
}

void shapelambda(Segment s, Color col, int N, struct dials d)
{
	// Create a shape in the center of the segment scaled to the area
	double area = abs(signed_area(s.boundary)) * s.scale * s.scale;
	Vertex mid = scale(centroid(s.boundary),s.scale);
	// Create a canvas to draw on!
	cairo_t* drawer = cairo_create(s.canvas);
	cairo_set_source_rgba(drawer, col.red, col.green, col.blue, 1.0);
	// Draw a circle if complexity is too low!
	if(N < 3) { draw_circle(drawer, area, mid, d); }
	// Draw a regular N-gon if complexity is high!
	else { draw_ngon(drawer, N, area, mid, d); }
	cairo_destroy(drawer);
}

Callback shape(Workspace* ws, Segment s, Brush b)
{
	// TODO: use orientation and entropy to modify stuff!
	// Check size, complexity, and palette constraints
	uint32_t palette_mask = 0;
	struct dials d;
	//double com_dial = -1.0;
	//double siz_dial = -1.0;
	//double ori_dial = -1.0;
	for(auto x : match_constraint(b.cons, s.constraint))
	{
		switch(x.type)
		{
			case CONS_PALETTE::PALETTE:
				palette_mask |= x.mask;
				break;
			case CONS::COMPLEXITY:
				d.com = accumulate_dial(d.com, x.dial);
				break;
			case CONS::SIZE:
				d.siz = accumulate_dial(d.siz, x.dial);
				break;
			case CONS::ORIENTATION:
				d.ori = accumulate_dial(d.ori, x.dial);
				break;
		}
	}
	
	// Choose a palette and color!
	Palette pal = pick_palette(ws, palette_mask);
	Color col = pick_color(ws, &pal, s.constraint);

	// Decide what shape depending on complexity!
	// Make a linear map between com and N = 10
	double maxN = 10.0;
	int N = int(d.com * maxN);

	// Decide if this brush is a good match
	bool usable = true;
	double match = 1.0; // TODO: smarter!

	Callback ret
	{
		.usable = usable,
		.match = match,
		.priority = b.priority,
		.callback = [=]() mutable -> void
		{
			//printf("Drawing Shape...\n");
			shapelambda(s, col, N, d);
		}
	};
	return ret;
}
