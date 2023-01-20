#include <stdio.h>
#include <stdlib.h>

// C++ imports
#include <vector>

// module imports
#include <cairo.h>
#include <cairo-svg.h>
#include "palette.h"
#include "render.h"
#include "brushes.h"
#include "constraints.h"

// Some brushes!

void solidlambda(Segment s, Color color)
{
	// Create the drawing context
	cairo_t* drawer = cairo_create(s.canvas);
	// Line width
	cairo_set_line_width(drawer, 10.0);
	// Color
	cairo_set_source_rgba(drawer, color.red, color.green, color.blue, 1);
	// Draw along the vertexes
	Vertex anchor = scale(s.boundary[0], s.scale);
	cairo_move_to(drawer, anchor.x, anchor.y);
	Vertex vert; 
	for(auto v : s.boundary)
	{
		vert = scale(v, s.scale);
		cairo_line_to(drawer, vert.x, vert.y);
	}
	cairo_close_path(drawer);
	cairo_fill(drawer);
	cairo_destroy(drawer);
}

Callback solid(Workspace* ws, Segment s, Brush b)
{
	// Find color pallette and decide if complexity is high enough
	// TODO: do something with generators to make this work...
	double match = -1.0;
	uint32_t palette_mask = 0;
	auto palette_list = match_constraint("palette", s.constraint);
	for(auto m : palette_list) { palette_mask |= m.mask; }

	Palette palette = pick_palette(ws, palette_mask);
	Color color = pick_color(ws, &palette, s.constraint);

	Callback ret
	{
		.usable = true,
		.match = match,
		.priority = b.priority,
		.callback = [=]() mutable -> void
		{
			//printf("Drawing Solid...\n");
			solidlambda(s, color);
		}
	};
	return ret;
}
