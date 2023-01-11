#include <stdio.h>
#include <math.h>

// C++ imports
#include <vector>
#include <set>
#include <map>

#include <iostream>

// module imports
#include "render.h"
#include "operators.h"
#include "constraints.h"

#include "geom.h"
using namespace geom;

std::set<uint32_t> fp_indexes(Workspace* ws, Operator op)
{
	std::set<uint32_t> ret;
	std::vector<Segment> cut = ws->cut();
	for(int i = 0; i < cut.size(); i++)
	{
		try
		{
			if(ws->op_cache[op].at(cut[i]) == -1)
			{
				ret.emplace(i);
			}
			else if(ws->op_cache[op][cut[i]] == i)
			{
				printf("FPIDX: %i -- %i\n",i,ws->op_cache[op][cut[i]]);
				ret.emplace(i);
			}
		} catch(std::out_of_range &e) { continue; }
	}
	return ret;
}

// Find the complexity-weighted distance to a focal point
// TODO: ADD OTHER CONSTRAINT WEIGHTS!
double fp_dis(Workspace* ws, Operator op, Segment s, uint32_t fp)
{
	Vertex dest = centroid(s.boundary);
	double ret = 0.0;
	Vertex o = centroid(ws->cut()[fp].boundary);
	Vertex dir = vec(o,dest);
	for(auto other_s : ws->cut())
	{
		std::vector<Vertex> itrs = intersect_ray_poly(
			o,dir,other_s.boundary);
		double cmp = match_accumulate_dial(
			CONS::COMPLEXITY,
			op.cons, other_s.constraint);
		switch(itrs.size())
		{
			case 0: continue;
			case 1:
				ret += arclen(Edge{itrs[0],dest}) * cmp;
				break;
			case 2:
				ret += arclen(Edge{itrs[0],itrs[1]}) * cmp;
				break;
			default: break;
		}
	}
	return ret;
}

// We want to have each segment associated with a focal point!
// Which focal point depends on distance, complexity, and orientation!
double fp_match(Workspace* ws, Operator op)
{
	// Find the focal points
	auto fp_idx = fp_indexes(ws, op);
	// If there are no focal points we can make one 100%
	if(fp_idx.size() == 0) { return 1.0; }
	// Otherwise we can find the complexity / distance measure
	double dis_max = 0.0;
	double miss_sum = 0.0;
	// A distance sum lambda
	auto dis_sum = [=](Workspace* ws, Operator op, Segment s) -> double
	{
		double sum = 0.0;
		for(auto idx : fp_idx) { sum += fp_dis(ws, op, s, idx); }
		return sum;
	};
	// Get the distance for each segment
	for(auto s : ws->cut())
	{
		double dis = dis_sum(ws, op, s);
		miss_sum += dis;
		dis_max = dis > dis_max ? dis : dis_max;
	}
	// If there is only one FP and segment to be had...
	if(dis_max == 0.0) { return 0.0; }
	double ret = (1.0 - (miss_sum / (ws->cut().size() * dis_max)));
	return ret;
}

std::vector<Segment> inclusion(Workspace* ws, Vertex p)
{
	std::vector<Segment> ret;
	for(auto s : ws->cut())
	{
		int wn = winding_number(p, s.boundary);
		if(wn != 0) { ret.push_back(s); }
	}
	return ret;
}

// Focal Point Closest
uint32_t fp_c(Workspace* ws, Operator op, std::set<uint32_t> fp, uint32_t s)
{
	double min = -1.0;
	uint32_t idx = -1;
	for(auto p : fp)
	{
		if(p == s) { continue; }
		double dis = fp_dis(ws, op, ws->cut()[s], p);
		idx = min == -1.0 || dis < min ? p : idx;
		min = min == -1.0 || dis < min ? dis : min;
	}
	return idx;
}

// TODO: weird that we don't need this... But it is expensive so yay?
/*
std::vector<std::pair<uint32_t,uint32_t>> fp_graph;
for(auto p : cand)
{
	std::pair<uint32_t,uint32_t> entry
	{
		p, fp_c(ws, op, cand, i)
	};
	fp_graph.push_back(entry);
}*/

/*
	// Check if a graph is connected
	auto fp_connected = [=](std::vector<std::pair<uint32_t,uint32_t>> map,
		uint32_t srt) -> bool
	{
		std::set<uint32_t> interior;
		std::set<uint32_t> edge {srt};
		while(edge.size() > 0)
		{
			uint32_t e = *edge.begin(); edge.erase(edge.begin());
			if(interior.count(e)) { continue; }
			else { interior.emplace(e); }
			for(auto pair : map)
			{
				uint32_t count_s = interior.count(pair.second);
				uint32_t count_f = interior.count(pair.first);
				if(pair.first == e && !count_s)
				{
					edge.emplace(pair.second);
				}
				if(pair.second == e && !count_f)
				{
					edge.emplace(pair.first);
				}
			}
		}
		return interior.size() == map.size();
	};
*/

// Focal Point Closest Connected
uint32_t fp_c_c(Workspace* ws, Operator op, std::set<uint32_t> fp)
{
	// Find the largest connected item
	double max = 0.0;
	uint32_t idx = -1;
	for(int i = 0; i < ws->cut().size(); i++)
	{
		if(!fp.count(i)) { continue; }
		double d = fp_dis(ws, op, ws->cut()[i], fp_c(ws, op, fp, i));
		idx = d > max ? i : idx;
		max = d > max ? d : max;
	}
	return idx;
}

uint32_t fp_add(Workspace* ws, Operator op, std::set<uint32_t> fp_idx)
{
	// If we have no focal point add a random one
	if(fp_idx.size() <= 0) { return int(ws->rand() * ws->cut().size()); }
	return fp_c_c(ws, op, fp_idx);
}

void fp_segment_add(Workspace* ws, Operator op, uint32_t fp_new)
{
	// Make a new degenerate segment in the center
	Segment ns = ws->cut()[fp_new];
	Vertex o = centroid(ns.boundary);
	printf("FOCAL POINT AT (%f,%f)\n",o.x,o.y);
	ws->addSegment(op, ns.layer,Polygon{o}, -1); // Add the focalpoint
	// TODO: add links!
}

void constraint_tweak(Workspace* ws, Operator op, Segment s, uint32_t fp)
{
	// Change orientation so it is more consistent
	std::cout << fp << ws->cut().size() << std::endl;
	Segment fp_seg = ws->cut()[fp];
	Vertex H = centroid(s.boundary);
	Vertex T = centroid(fp_seg.boundary);
	//printf("\tANGLES?!?!?\n");
	double phi = angle({1.0,0.0}, vec(H,T));
	double ori = M_PI * match_accumulate_dial(
		CONS::ORIENTATION, op.cons, s.constraint);

	double tweak = 1.0 - abs(cos((phi - ori) * 2));
	tweak = cos(phi - ori) < 0.0 ? tweak : -1.0 * tweak;
	for(auto m : match_constraint(op.cons, s.constraint))
	{
		if(m.type == CONS::ORIENTATION)
		{
			Constraint con = s.constraint[m.i];
			double unbound = con.dial + tweak;
			double next = 0.0;
			if(unbound < 0.0) { next = 1.0 + unbound; }
			else if (unbound > 1.0) { next = -1.0 + unbound; }
			else { next = unbound; }
			ws->setConstraint(op, s, {{con.name, 0, unbound}});
		}
	}
	//printf("\t\tPHI, DIAL: %f -- %f -> %f -> %f\n",
	//	phi, ori, cos(phi - ori), tweak);
}

// Tweak the constraints on segments, assign them to new FP, basically make
// the art piece more pleasing to a wandering eye.
// TODO: consider other constraints?
void constraint_tweaks(Workspace* ws, Operator op, std::set<uint32_t> fp)
{
	if(fp.size() <= 0) { return; } // Need at least one focal point
	// Tweak the constraints around the best focal point
	int i = 0;
	for(auto s : ws->cut())
	{
		// Find the best focal point
		uint32_t idx = fp_c(ws, op, fp, i);
		// Skip focal points and errors
		if(idx == -1 || fp.count(i)) { i++; continue; }
		// Is there a cached focal point?
		if(ws->op_cache[op].count(s) && ws->op_cache[op][s] != -1)
		{
			uint32_t cache_idx = ws->op_cache[op][s];
			double dis = fp_dis(ws, op, s, idx);
			double cache_dis = fp_dis(ws, op, s, cache_idx);
			// Decide if cache needs to change
			// TODO: fiddle with this?
			if(cache_dis * 0.8 < dis) { idx = cache_idx; }
		}
		constraint_tweak(ws, op, s, idx); // Do the actuall tweaks
		ws->op_cache[op][s] = idx; // Update cache
		i++; // Update the index counter
	}
}

void fplambda(Workspace* ws, Operator op)
{
	printf("FOCAL POINTS...\n");
	std::set<uint32_t> fp_idx = fp_indexes(ws, op);

	//printf("\tNUM FOCAL POINTS: %i\n", fp_idx.size());

	uint32_t fp_new = fp_add(ws, op, fp_idx);
	if(fp_new != -1) { fp_segment_add(ws, op, fp_new); }
	// Tweak constraints to make them better!
	constraint_tweaks(ws, op, fp_idx);
}

Callback focal(Workspace* ws, Operator op)
{
	// TODO: make a distance map to send to fp_match and bind to fplambda!
		// This is a factor 2 speedup
	// TODO: make a connectivity graph of segments in plane, attach to WS
		// This is a factor N**2 -> log(N) speedup
	// TODO: PALETTE TWEAKS! - Make palettes more bold close to a FP!
	double match = fp_match(ws, op);
	Callback ret
	{
		.usable = match != 0.0,
		.match = match,
		.priority = 1.0,
		.callback = [=]() mutable -> void { fplambda(ws, op); }
	};
	return ret;
}
