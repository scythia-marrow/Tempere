#include <stdio.h>
#include <stdlib.h>

// C++ imports
#include <vector>

// module imports
#include <cairo.h>
#include "palette.h"
#include "render.h"
#include "brushes.h"
#include "constraints.h"

// Some brushes!

void solidlambda(Segment* s, Color color)
{
	// Create the drawing context
	cairo_t* drawer = cairo_create(s->canvas);
	// Line width
	cairo_set_line_width(drawer, 10.0);
	//cairo_set_line_width(drawer, 30.0);
	//cairo_set_source_rgba(drawer, color.red, color.green, color.blue, 1);
	// Draw along the vertexes
	Vertex anchor = scale(s->boundary[0], s->scale);
	cairo_move_to(drawer, anchor.x, anchor.y);
	Vertex vert; 
	for(auto v : s->boundary)
	{
		vert = scale(v, s->scale);
		cairo_line_to(drawer, vert.x, vert.y);
		//cairo_save(drawer);
		//cairo_set_source_rgba(drawer, 0.8,0.8,0.8,0.6);
		//cairo_arc(drawer, vert.x, vert.y, 30.0, 0.0, 2.0 * M_PI);
		//cairo_restore(drawer);
	}
	cairo_line_to(drawer, anchor.x, anchor.y);
	cairo_set_source_rgba(drawer, color.red, color.green, color.blue, 1);
	cairo_fill(drawer);
	cairo_destroy(drawer);
}

Callback solid(Workspace* ws, Segment* s, Brush b)
{
	// Find color pallette and decide if complexity is high enough
	// TODO: do something with generators to make this work...
	double match = -1.0;
	uint32_t palette_mask = 0;
	for(auto x : match_constraint(b.cons, s->constraint))
	{
		switch(x.type)
		{
			case CONS::COMPLEXITY:
				match = accumulate_dial(match, x.dial);
				break;
			case CONS_PALETTE::PALETTE:
				palette_mask |= x.mask;
				break;
		}
	}

	Palette palette = pick_palette(ws, palette_mask);
	Color color = pick_color(ws, &palette, s->constraint);

	Callback ret
	{
		.usable = true,
		.match = match,
		.priority = b.priority,
		.callback = [=]() mutable -> void
		{
			solidlambda(s, color);
		}
	};
	return ret;
}
