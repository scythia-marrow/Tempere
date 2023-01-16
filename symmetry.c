#include <stdio.h>

// C++ imports
#include <cassert>
#include <vector>
#include <map>
#include <algorithm>

// module imports
#include "geom.h"
#include "render.h"
#include "operators.h"

// DEBUG
#include <iostream>

int decide_symmetry(Segment s, Operator op)
{
	// Decide upon a symmetry
	double sym;
	sym = match_accumulate_dial(CONS::COMPLEXITY, op.cons, s.constraint);
	return int(sym * 8);
}

std::vector<Segment> cut_mark_symmetry(Workspace* ws, Operator op)
{
	std::vector<Segment> ret;
	for(auto s : ws->cut())
	{
		// Check if segment is marked
		uint32_t mark = ws->op_cache[op][s];
		if(mark == 1) { continue; }
		// Check if segment is complex enough
		if(decide_symmetry(s, op) < 2) { continue; }
		ret.push_back(s);
	}
	// Sort by maximum area
	auto arealamb = [](const Segment &s1,const Segment &s2) -> bool
	{
		return geom::area(s1.boundary) > geom::area(s2.boundary);
	};
	std::sort(ret.begin(),ret.end(),arealamb);
	return ret;
}

std::vector<Edge> radial_segments(Segment max_seg, Vertex mid, int N)
{
	// Create a direction vector in an even circle
	double phi = (2.0 * M_PI) / N;
	std::vector<Vector> direction;
	// TODO: change
	for(int i = 0; i < N; i++)
	{
		Vector dir {(double)sin(phi*i), (double)cos(phi*i)};
		direction.push_back(dir);
	}
	// For each directon find radial intersections
	std::vector<Edge> ret;
	for(auto dir : direction)
	{
		// TODO: fix convex / complex case
		int count = 0;
		for(auto e : edgeThunk(max_seg.boundary))
		{
			Optional<Vertex> intO = intersect_ray_line(e,mid,dir);
			if(intO.is)
			{
				ret.push_back(Edge{mid,intO.dat});
				count++;
			}
		}
	}
	return ret;
}

void symmetrylambda(Workspace* ws, Operator op, Segment max_seg, int N)
{
	uint32_t layer = max_seg.layer; // Find the segment layer
	Vertex mid = centroid(max_seg.boundary); // Find the segment centroid
	// Create a set of N radial lines that end at intersection points
	std::vector<Edge> lines = radial_segments(max_seg, mid, N);
	for(auto e : lines)
	{
		// Add segment with no mark
		std::vector<Edge> line = {Edge{e.head,e.tail},{e.tail,e.head}};
		ws->addSegment(op, layer, polygonThunk(line), 0);
	}
	// Make complexity constraints?
}

Callback symmetry(Workspace* ws, Operator op)
{
	// Find the usable segment with maximum perimeter
	std::vector<Segment> segCand = cut_mark_symmetry(ws, op);
	bool usable = false;
	Segment max_seg;
	if(segCand.size() > 0)
	{
		max_seg = segCand[0];
		usable = true;
	}
	// Parameters for the callback
	//bool usable = N < 2 ? false : true; // Cannot break into <2 pieces
	// TODO: make this smarter
	double match = (1.0 / log(1.0 + ws->cut().size()));

	// The callback!
	Callback ret
	{
		.usable = usable,
		.match = match,
		.priority = 1.0,
		.callback = [=]() mutable -> void
		{
			// If we proceed, mark this segment as used
			// ws->op_cache[op][*max_seg] = 1;
			int N = decide_symmetry(max_seg, op);
			printf("SYMMETRY (%d)...\n",N);
			symmetrylambda(ws, op, max_seg, N);
		}
	};

	return ret;
}
