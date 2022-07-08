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
double line_number(Segment s, LINE_STATE state)
{
	// Decide the number of lines
	double area = abs(signed_area(s.boundary));
	area = area < 1.0 ? 1.0 : area;
	double N = ((s.scale * state.cmp) / (area * state.siz));
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

// Because the vertexes created only from constraints, they are the same!
std::vector<Vertex> vertexes(Workspace* ws, Segment s, Brush b)
{
	/*
	double size = match_accumulate_dial(
		CONS::SIZE, b.cons, s->constraint);
	double cmp = match_accumulate_dial(
		CONS::COMPLEXITY, b.cons, s->constraint);
	double ori = match_accumulate_dial(
		CONS::ORIENTATION, b.cons, s->constraint);
	*/
	std::vector<Vertex> ret;
	// The first vertex is the centroid
	ret.push_back(scale(centroid(s.boundary), s.scale));
	// TODO: this!
	/*for(int i = 1; i < ws->br_cache[b][sg]; i++)
	{
		
	}*/
	// Create a number of vertexes equal to the thingymabober
	return ret;
}

std::vector<Vertex> choose_vertex(Workspace* ws, LINE_STATE s, uint32_t N)
{
	// Vector to return
	std::vector<Vertex> ret;
	// Find other segments with lines to them
	std::vector<uint64_t> vertex = marked_ids(ws, s.brush);
	// If there are no other segments
	if(vertex.size() == 0) { return ret; }
	// The weights
	std::vector<std::pair<double, Vertex>> sorted;
	// Insertion sort, nice and clean
	/*
	auto insertsort = [&](Vertex vrt, double weight) -> void
	{
		int i; for(i = 0; sorted[i].first < weight; i++); // Seek
		sorted.insert(sorted.begin() + i, {weight, vrt});
	};*/
	// Calculate weight of each vertex based on orientation, dist, ect.
	// TODO: start here!
	// Return the first N from sorted
	if(sorted.size() < N)
	{
		printf("NOT ENOUGH! %i\n", N);
		return ret;
	}
	for(uint32_t i = 0; i < N; i++) { ret.push_back(sorted[i].second); }
	return ret;
}

// Some brushes!
void linelambda(Workspace* ws, Segment sg, LINE_STATE s)
{
	// Store the number of vertexes in this segment
	bool exists = ws->br_cache[s.brush].count(sg);
	uint32_t next = !exists ? 1 : ws->br_cache[s.brush][sg] + 1;
	// Add a new vertex if there is enough complexity to justify it
	if(next < line_number(sg, s)) { ws->br_cache[s.brush][sg] = next; }
	int num = ws->br_cache[s.brush][sg];
	// Draw lines if there are enough matches to justify them
	std::vector<Vertex> head = vertexes(ws, sg, s.brush);
	std::vector<Vertex> tail = choose_vertex(ws, s, num);
	std::cout << "LINE DRAW " << head.size() << " " << tail.size() << std::endl;
	if(head.size() == 0 || tail.size() == 0) { return; }

	cairo_t* drawer = cairo_create(sg.canvas);
	double size = s.siz * 10.0;
	//double area = abs(signed_area(sg.boundary));
	size = size < 1.0 ? 1.0 : size;

	Color c = s.color;
	cairo_set_line_width(drawer, size);
	cairo_set_source_rgba(drawer, c.red, c.green, c.blue, 1.0);

	// Draw a number of lines from different starting points
	for(int i = 0; i < num; i++)
	{
		// Choose a start and end point
		Vertex vrt_head = head[int(ws->rand() * head.size())];
		Vertex vrt_tail = tail[int(ws->rand() * tail.size())];
		cairo_move_to(drawer, vrt_head.x, vrt_head.y);
		cairo_line_to(drawer, vrt_tail.x, vrt_tail.y);
	}

	cairo_stroke(drawer);
	cairo_destroy(drawer);
}

Callback line(Workspace* ws, Segment s, Brush b)
{
	// Ensure the cache is constructed
	// ensure_cache(ws, b);
	// Pick the line(s) palette
	uint32_t palette_mask = 0;
	Palette palette = pick_palette(ws, palette_mask);
	Color color = pick_color(ws, &palette, s.constraint);

	// Create the costate for the lambda
	LINE_STATE state = 
	{
		.brush = b,
		.color = color,
		.siz = match_accumulate_dial(
			CONS::SIZE, b.cons, s.constraint),
		.cmp = match_accumulate_dial(
			CONS::COMPLEXITY, b.cons, s.constraint),
		.ori = match_accumulate_dial(
			CONS::ORIENTATION, b.cons, s.constraint),
	};

	// Find the match
	double N = line_number(s, state);
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
