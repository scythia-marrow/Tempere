#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// C++ imports
#include <vector>
#include <deque>
#include <set>
#include <map>

// module imports
#include "geom.h"
#include "render.h"
#include "operators.h"
#include "constraints.h"

typedef std::vector<std::pair<uint16_t,uint16_t>> GRAPH;

// How gradients are stored in the op cache
union GRAD
{
	uint32_t word;
	struct
	{
		uint16_t id;
		uint16_t pt;
	};
};

GRAD gradCacheGet(Workspace* ws, Operator op, Segment* s)
{
	if(ws->op_cache[op].count(s)) { return {.word = ws->op_cache[op][s]}; }
	return {.word = (uint32_t)-1};
}

void gradCacheSet(Workspace* ws, Operator op, Segment* s, GRAD g)
{
	ws->op_cache[op][s] = g.word;
}

std::map<uint64_t,GRAPH> readGrads(Workspace* ws, Operator op) {
	std::map<uint64_t,GRAPH> ret;
	int i = 0;
	for(auto s : ws->segment)
	{
		GRAD grad = gradCacheGet(ws, op, s);
		if(grad.word == (uint32_t)-1) { continue; }
		ret[grad.id].push_back({i,grad.pt});
		i++;
	}
	return ret;
}

// Full link graph creation with MST ect. Use djkstra for MST.
struct DIJKSTRAS_RET { GRAPH link; GRAPH mst; uint64_t span; };
DIJKSTRAS_RET linkGraph(Workspace* ws)
{
	// The return struct things
	GRAPH mst;
	GRAPH lnk;
	uint64_t max = 0;

	// Make ID map
	int i = 0;
	std::map<Segment*,uint64_t> id;
	for(auto s : ws->segment) { id[s] = i; i++; }

	// Do djkstras algorithm for graph and MST
	std::map<uint64_t,uint64_t> dijkstra; dijkstra[0] = 0;
	std::deque<uint64_t> leaf {0};
	std::set<uint64_t> core;
	while(leaf.size() > 0)
	{
		uint64_t next = leaf.front(); leaf.pop_front();
		core.insert(next);
		for(auto l : ws->segment[next]->link)
		{
			uint64_t link = id[l];
			lnk.push_back({next,link});
			// If we bump into another leaf set the span
			if(!dijkstra.count(link))
			{
				uint64_t span = dijkstra[next]+1;
				max = span > max ? span : max;
				dijkstra[link] = span;
				mst.push_back({next,link});
				leaf.push_back(link);
			}
		}	
	}
	return {lnk,mst,max};
}

uint32_t grdcon(Workspace* ws, Operator op, GRAPH graph)
{
	// Find the gradient constraint
	double dis = -1.0;
	uint32_t constraint = -1;
	for(auto con : op.cons)
	{
		double min = -1.0;
		double max = -1.0;
		for(auto edge : graph)
		{
			uint64_t id = (uint64_t)edge.first;
			double val = match_accumulate_dial(
				con.second,
				op.cons,
				ws->segment[id]->constraint);
			max = (max == -1.0 || val > max) ? val : max;
			min = (min == -1.0 || val < max) ? val : min;
		}
		double diff = max - min;
		dis = (dis == -1.0 || diff > dis) ? diff : dis;
		if(dis == diff) { constraint = con.second; }
	}
	return constraint;
}

struct GRAPHSTAT { double min; double max; double minC; double maxC; double s;};
GRAPHSTAT graph_stats(Workspace* ws, Operator op, uint32_t c, GRAPH graph)
{
	double min = -1.0;
	double max = -1.0;
	double maxC = -1.0;
	double minC = -1.0;
	double count = 0.0;
	for(auto g : graph)
	{
		uint16_t id = g.second;
		double val = match_accumulate_dial(
			c, op.cons, ws->segment[id]->constraint);
		min = (min == -1.0 || val < min) ? val : min;
		max = (max == -1.0 || val > max) ? val : max;
		if(min == val) { minC = count; } 
		if(max == val) { maxC = count; }
		count += 1.0;
	}
	//printf("GRAPH STAT: %f %f -- %f %f\n", min, max, minC, maxC);
	return {min, max, minC, maxC, count};
}

double edge_target(double count, GRAPHSTAT stat)
{
	// Distances to each FP
	double maxD = stat.maxC < (stat.s - stat.maxC) ?
		stat.maxC : (stat.s - stat.maxC);
	double minD = stat.minC < (stat.s - stat.minC) ?
		stat.minC : (stat.s - stat.minC);
	double midD = abs(stat.maxC - stat.minC) / 2.0;
	// Find the largest distance
	double distD = 0.0;
	std::vector<double> candD { maxD, midD, minD };
	for(auto d : candD) { distD = d > distD ? d : distD; } // Max dist
	if(distD == 0.0) { return stat.max; }
	// If there is a maximum distance make a delta
	double diff = stat.max - stat.min;
	double delta = diff / distD;
	// Find the distance to nearest FP
	double distC = stat.s;
	std::vector<double> candC {abs(stat.minC-count),abs(stat.maxC-count)};
	for(auto c : candC) { distC = c < distC ? c : distC; }
	// Find the tweak sign
	double sign = 0.0;
	sign = abs(stat.minC - count) < abs(stat.maxC - count) ? 1.0 : -1.0;
	// The target depends on which FP is closest
	double tgt = sign == 1.0 ?
		stat.min + (delta * distC) : stat.max - (delta * distC);
	return tgt;
}

// We have a set of gradients dependent on the thingy
double grdmatch(Workspace* ws, Operator op, std::map<uint64_t,GRAPH> gradient)
{
	// Eval all gradients
	for(auto grad : gradient)
	{
		uint32_t constraint = grdcon(ws, op, grad.second);
		GRAPHSTAT stat = graph_stats(ws, op, constraint, grad.second);
		// Evaluate the constraint by finding the SD of differences
		std::vector<double> diff;
		double count = 0.0;
		for(auto edge : grad.second)
		{
			uint32_t id = edge.second;
			double val = match_accumulate_dial(
				constraint,
				op.cons,
				ws->segment[id]->constraint);
			double tgt = edge_target(count, stat);
			diff.push_back(val - tgt);
			count += 1.0;
		}
	}

	// See how well gradients are created
	return 1.0;
}

void update_chains( Workspace* ws, Operator op, std::map<uint64_t,GRAPH> grad)
{
	// Update previous chains
	uint16_t  max = -1;
	for(auto graph : grad)
	{
		// Find the maximum thingy
		max = (max == -1 || graph.first > max) ? graph.first : max;
		// Tweak the constraints
		uint32_t constraint = grdcon(ws, op, graph.second);
		GRAPHSTAT stat = graph_stats(ws, op, constraint, graph.second);
		// Now that we know the statistics we want to update stuff
		double count = 0.0;
		for(auto g : graph.second)
		{
			double target = edge_target(count, stat);
			uint32_t id = g.second;
			Segment* seg = ws->segment[id];
			for(auto m : match_constraint(op.cons, seg->constraint))
			{
				// Move it closer to the target TODO: scale!
				if(m.type == constraint)
				{
					double diff = target - m.dial;
					double low = m.dial;
					double hgh = 1.0 - m.dial;
					double scale = diff <= 0.0 ? low : hgh;
					double update =
						m.dial +
						(scale * diff * 0.3);
					seg->constraint[m.i].dial = update;
				}
			}
			count += 1.0;
		}
	}
}

GRAPH find_chain(Workspace* ws, DIJKSTRAS_RET dijk)
{
	uint16_t place = ws->rand() * (ws->segment.size() - 1);
	GRAPH cand;
	while(true)
	{
		// Find all connections
		std::vector<uint16_t> leaf;
		for(auto s : dijk.mst)
		{
			if(s.first != place) { continue; }
			uint16_t e = s.second;
			bool breaker = true;
			for(auto edge : cand)
			{
				if(e == edge.first || e == edge.second)
				{
					breaker = false;
					break;
				}
			}
			if(breaker) { leaf.push_back(e); }
		}
		if(leaf.size() == 0) { break; }
		// Choose random leaf from MST
		uint64_t rnd = ws->rand() * (leaf.size() - 1);
		uint16_t next = leaf[rnd];
		cand.push_back({place, next});
		place = next;
	}
	return cand;
}

void grdlambda(
	Workspace* ws, Operator op,
	std::map<uint64_t,GRAPH> gradient, DIJKSTRAS_RET dijk)
{
	// Update chains
	update_chains(ws, op, gradient); //TODO: RETURN thingy!
	// Use the MST to find a chain
	GRAPH cand = find_chain(ws, dijk);
	// Find the next thingy
	uint64_t max = 0;
	for(auto x : gradient) { max = x.first > max ? x.first : max; }
	// Store the gradient in the op cache!
	if(cand.size() <= 4) { return; }
	for(auto g : cand)
	{
		GRAD grd; grd.id = max + 1; grd.pt = g.second;
		gradCacheSet(ws, op, ws->segment[g.first], grd);
	}
}

Callback gradient(Workspace* ws, Operator op)
{
	// Graph of previous gradients
	std::map<uint64_t,GRAPH> grd = readGrads(ws, op);
	// Full segment connectivity graph
	auto dijk = linkGraph(ws);

	Callback ret
	{
		.usable = (dijk.span > 1), //TODO: maybe 2 is better here?
		.match = grdmatch(ws, op, grd),
		.priority = 1.0,
		.callback = [=]() mutable -> void
		{
			grdlambda(ws, op, grd, dijk);
		}
	};
	return ret;
}
