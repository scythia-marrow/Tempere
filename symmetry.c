#include <stdio.h>
#include <assert.h>

// C++ imports
#include <vector>
#include <map>

// module imports
#include "render.h"
#include "operators.h"

int decide_symmetry(Segment s, Operator op)
{
	// Decide upon a symmetry
	int N = 0;
	double sym;
	sym = match_accumulate_dial(CONS::COMPLEXITY, op.cons, s.constraint);
	return int(sym * 8);
}

Segment max_segment(Workspace* ws, Operator op)
{
	// Find the largest segment (by perimeter)
	double max_perim = 0.0;
	Segment* max_seg = new Segment;
	for(auto s : ws->cut())
	{
		// Check if segment is marked
		uint32_t mark = ws->op_cache[op][s];
		if(mark == 1) { continue; }
		// Check if segment is complex enough
		if(decide_symmetry(s, op) < 2) { continue; }
		// Otherwise calculate the preim
		std::vector<Vertex> bound = s.boundary;
		double perim = perimeter(bound);
		if(perim >= max_perim)
		{
			max_perim = perim;
			max_seg = new Segment{s};
		}
	}
	//printf("\tMax Segment: %p\n", max_seg);
	return *max_seg;
}

// Map all indexes to one or more symmetrical directions
std::map<int,std::vector<int>> find_interior(Segment s, Vertex mid, int N)
{
	// The angle between lines
	double ang = (2.0 * M_PI) / N;
	// Check all boundary segments for intersections
	uint32_t size = s.boundary.size();
	// N segments needs N lines with as many intersection points
	std::map<int,std::vector<int>> interior;
	Vertex D_h;
	Vertex D_t;
	for(int h = 0; h < N; h++)
	{
		int t = (N + h + 1) % N;
		// Two angles sweep out an arc
		D_h = {1.0001*sin(ang * h) , 0.9999*cos(ang * h)};
		D_t = {0.9999*sin(ang * t) , 1.0001*cos(ang * t)};
		// If the vertex is part of an arc, store it!
		for(uint32_t i = 0; i < size; i++)
		{
			Vertex v = s.boundary[i];
			double angle_h = angle(D_h,vec(mid,v));
			double angle_t = angle(D_t,vec(mid,v));
			if(angle_h < ang && angle_t < ang)
			{
				interior[h].push_back(i);
			}
		}
	}
	return interior;
}


// I do like thunks, but...
struct edge {
	Vertex inter;
	int head;
	int tail;
};

std::map<int,std::vector<struct edge>> find_edge(Segment s, Vertex mid, int N)
{
	// The angle between lines
	double ang = (2.0 * M_PI) / N;
	// Check all boundary segments for intersections
	int size = s.boundary.size();
	// N segments needs N lines with as many intersection points
	std::map<int,std::vector<struct edge>> edges;
	Vertex D_h;
	Vertex D_t;
	for(int n_h = 0; n_h < N; n_h++)
	{
		int n_t = (N + n_h + 1) % N;
		// Two angles sweep out an arc
		D_h = {1.0001*sin(ang*n_h),0.9999*cos(ang*n_h)};
		D_t = {0.9999*sin(ang*n_t),1.0001*cos(ang*n_t)};
		// Check all edges for intersections
		for(int v_h = 0; v_h < size; v_h++)
		{
			int v_t = (size + v_h + 1) % size;
			Vertex vrt_h = s.boundary[v_h];
			Vertex vrt_t = s.boundary[v_t];
			Vertex inter_h =
				intersect_ray_line(mid, D_h, vrt_h, vrt_t);
			Vertex inter_t =
				intersect_ray_line(mid, D_t, vrt_h, vrt_t);
			if(!eq(inter_h, mid))
			{
				struct edge e_h { inter_h, v_h, v_t };
				edges[n_h].push_back(e_h);
			}
			if(!eq(inter_t, mid))
			{
				struct edge e_t { inter_t, v_h, v_t };
				edges[n_h].push_back(e_t);
			}
			if(edges[n_h].size() >= 2) { continue; }
		}
	}
	return edges;
}


std::pair<int,std::vector<Vertex>> interior_chain(
	Segment s,
	std::vector<Vertex> start,
	std::vector<int> in,
	std::vector<struct edge> edge,
	int dir, int init)
{
	// The chain to return
	std::vector<Vertex> chain;
	for(auto v : start) { chain.push_back(v); }
	//printf("Edges!: \n\t");
	/*for(auto e : edge)
	{
		printf("%i -> %i ",e.head,e.tail);
	}
	//printf("\n");
	//printf("Inners!: \n\t");
	for(auto i : in)
	{
		printf(" %i ", i);
	}
	printf("\n");*/
	// A cool for loop
	int e = 1;
	int size = s.boundary.size();
	//printf("Chain: \n\t ");
	int n = 0;
	int k = 0;
	for(int c = init ;; c += dir)
	{
		k++;
		if(k > 100)
		{
			printf("ERROR! c = %i, e = %i, size = %i, dir = %i\n",
				n, c, e, size, dir);
			break;
		}
		// Transform chain index to boundary index
		int i = (size + c) % size;
		// If the index is not on an edge check if it is interior
		if(i != edge[e].head && i != edge[e].tail)
		{
			// If the index is in the interior, add it
			int P = std::find(in.begin(),in.end(),i) - in.begin();
			if(P < (int)in.size())
			{
				chain.push_back(s.boundary[i]);
				continue;
			}
			n++;
			continue;
		}
		// I love ternary expressions!
		int real_h = (i == edge[e].head) ? edge[e].head : edge[e].tail;
		int real_t = (i == edge[e].head) ? edge[e].tail : edge[e].head;
		// Push back the head and the intersection point
		chain.push_back(s.boundary[real_h]);
		chain.push_back(edge[e].inter);
		// If this is the last edge stop the segment
		e++;
		if(e >= (int)edge.size()) { break; } // No combining ++ and >=
		chain.push_back(s.boundary[real_t]);
	}
	return std::pair(n,chain);
}

// Find the next link of the chain by seeking through the segment
// in both directions and choosing the shorter one.
// TODO: add support for convex and disjointed segments!
std::vector<Vertex> vertex_chain(
	Segment s,
	Vertex mid,
	std::vector<int> interior,
	std::vector<struct edge> edge)
{

	std::vector<Vertex> chain_init;

	// Both chains start at the middle and move out to the first
	// edge vector!
	chain_init.push_back(mid);
	chain_init.push_back(edge[0].inter);

	// If there are no interior points we are done!
	if(interior.size() < 1)
	{
		for(auto e : edge)
		{
			chain_init.push_back(e.inter);
		}
		return chain_init;
	}
	
	// However they follow opposite directions!
	int frnt_dir = edge[0].head - edge[0].tail;
	int back_dir = edge[0].tail - edge[0].head;
	int frnt_i = frnt_dir < 0 ? edge[0].head : edge[0].tail;
	int back_i = back_dir < 0 ? edge[0].head : edge[0].tail;
	
	std::pair<int,std::vector<Vertex>> front_chain	=
		interior_chain(s,chain_init, interior, edge, frnt_dir, frnt_i);
	std::pair<int,std::vector<Vertex>> back_chain =
		interior_chain(s,chain_init, interior, edge, back_dir, back_i);

	std::vector<Vertex> chain =
		front_chain.first < back_chain.first ?
			front_chain.second : back_chain.second;

	//printf("SIZES: %i, %i\n", front_chain.first, back_chain.first);

	/*printf("FRONT CHAIN: \n");
	for(auto v : front_chain.second)
	{
		printf("\t (%f,%f)\n",v.x,v.y);
	}
	printf("BACK CHAIN: \n");
	for(auto v : back_chain.second)
	{
		printf("\t (%f,%f)\n",v.x,v.y);
	}*/
	return chain;
}

void symmetrylambda(Workspace* ws, Operator op, Segment max_seg, int N)
{
	uint32_t foreground = max_seg.layer + 1; // Move forward
	Vertex mid = centroid(max_seg.boundary); // Find the segment centroid
	double phi = (2.0 * M_PI) / N; // The angle between lines.
	std::map<int,std::vector<int>> interior =
		find_interior(max_seg, mid, N);
	std::map<int,std::vector<struct edge>> edge =
		find_edge(max_seg, mid, N);

	
	// Create new segments and boundaries
	std::vector<Segment> shards;
	for(int h = 0; h < N; h++)
	{
		// IF WE CANT FIND EDGES TODO: BUGFIX!
		if(edge[h].size() == 0) { continue; }
		assert(edge[h].size() != 0);
		//printf("SEGMENT %i:\n", h);
		int t = (N + h + 1) % N;
		Vertex D_h {1.0001*(double)sin(phi*h),0.9999*(double)cos(phi*h)};
		Vertex D_t {0.9999*(double)sin(phi*t),1.0001*(double)cos(phi*t)};
		std::vector<Vertex> dir {D_h, D_t};
		// Find the vertex intersection of the thingies
		std::vector<Vertex> chain = vertex_chain(
			max_seg, mid, interior[h], edge[h]);	
		// Append the new segment with no mark
		ws->addSegment(op, foreground, chain, 0);
	}
}

Callback symmetry(Workspace* ws, Operator op)
{
	// Find the usable segment with maximum perimeter
	Segment max_seg = max_segment(ws, op);
	bool usable = true;
	// Parameters for the callback
	//bool usable = N < 2 ? false : true; // Cannot break into <2 pieces
	double match = (1.0 / log(1.0 + ws->cut().size())); // TODO: make this smarter

	// The callback!
	Callback ret
	{
		.usable = usable,
		.match = match,
		.priority = 1.0,
		.callback = [=]() mutable -> void
		{
			printf("SYMMETRY...\n");
			// If we proceed, mark this segment as used
			ws->op_cache[op][max_seg] = 1;
			int N = decide_symmetry(max_seg, op);
			symmetrylambda(ws, op, max_seg, N);
		}
	};

	return ret;
}
