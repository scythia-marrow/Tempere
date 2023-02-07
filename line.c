#include <stdio.h>
#include <stdlib.h>

// C++ imports
#include <vector>
#include <iostream>


// module imports
#include <cairo.h>
#include "palette.h"
#include "render.h"
#include "brushes.h"
#include "constraints.h"

typedef struct linestate
{
	Brush brush;
	Color color;
	double siz;
	double cmp;
	double ori;
} LINE_STATE;

// Matching code!
double line_number(Workspace* ws, Segment s, LINE_STATE state)
{
	// Decide the number of lines, depending on number of neighbors
	double area = abs(signed_area(s.boundary));
	area = area < 1.0 ? 1.0 : area;
	double N = ((s.scale * state.cmp) / (area * state.siz));
	// Get the number of neighbors
	uint32_t neighbors = ws->geomRel(s).size();
	/*
	if(neighbors < N)
	{
		printf("NEIGHBORS %d\n\t",neighbors);
		printf("CENTERS");
		for(auto n : ws->geomRel(s))
		{
			printf("(%f,%f) ",geom::midpoint(n.boundary).x,geom::midpoint(n.boundary).y);
		}
		printf("\n");
		return neighbors;
	}//*/
	return N;
}

// TODO: make multiple line ends in a segment? May be very interesting.
std::vector<uint64_t> marked_ids(Workspace* ws, Brush b)
{
	std::vector<uint64_t> ret;
	std::vector<Segment> seg = ws->cut();
	for(uint32_t i = 0; i < seg.size(); i++)
	{
		if(ws->br_cache[b].count(seg[i])) { ret.push_back(i); }
	}
	return ret;
}

std::vector<Segment> vecThunk(std::set<Segment> item)
{
	std::vector<Segment> ret = {};
	for(auto i : item) { ret.push_back(i); }
	return ret;
}

// Some brushes!
void linelambda(Workspace* ws, Segment sg, LINE_STATE s)
{
	// Store the number of vertexes in this segment
	bool exists = ws->br_cache[s.brush].count(sg);
	uint32_t next = !exists ? 1 : ws->br_cache[s.brush][sg] + 1;
	// Add a new vertex if there is enough complexity to justify it
	if(next < line_number(ws, sg, s)) { ws->br_cache[s.brush][sg] = next; }
	else { return; }
	// Draw another line if there are enough matches to justify them
	auto start = geom::centroid(sg.boundary);
	auto end = geom::centroid(vecThunk(ws->geomRel(sg))[next-1].boundary);

	cairo_t* drawer = cairo_create(sg.canvas);
	double size = s.siz * 10.0;
	//double area = abs(signed_area(sg.boundary));
	size = size < 1.0 ? 1.0 : size;

	Color c = s.color;
	cairo_set_line_width(drawer, size);
	cairo_set_source_rgba(drawer, c.red, c.green, c.blue, 1.0);
	cairo_move_to(drawer, ws->scale() * start.x, ws->scale() * start.y);
	cairo_line_to(drawer, ws->scale() * end.x, ws->scale() * end.y);
	cairo_stroke(drawer);
	cairo_destroy(drawer);
}

Callback line(Workspace* ws, Segment s, Brush b)
{
	// Ensure the cache is constructed
	// ensure_cache(ws, b);
	// Pick the line(s) palette
	uint32_t palette_mask = 0;
	for(auto m : match_constraint("palette",s.constraint))
	{
		palette_mask |= m.mask;
	}
	Palette palette = pick_palette(ws, palette_mask);
	Color color = pick_color(ws, &palette, s.constraint);

	// Create the costate for the lambda
	auto sizmatch = match_constraint("size",s.constraint);
	auto cmpmatch = match_constraint("complexity",s.constraint);
	auto orimatch = match_constraint("orientation", s.constraint);

	LINE_STATE state = 
	{
		.brush = b,
		.color = color,
		.siz = distribution(sizmatch)(ws->rand),
		.cmp = distribution(cmpmatch)(ws->rand),
		.ori = distribution(orimatch)(ws->rand)
	};

	// Find the match
	double N = line_number(ws, s, state);
	double match = N <= 0.0 ? 0.0 : (1.0 / (1.0 + log(N)));

	Callback ret
	{
		.usable = match != 0.0 ? true : false,
		.match = match,
		.priority = b.priority,
		.callback = [=]() mutable -> void
		{
			//printf("Drawing Line...\n");
			linelambda(ws, s, state);
		}
	};
	return ret;
}
