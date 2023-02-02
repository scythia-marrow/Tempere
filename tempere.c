#include "geom.h"
#include "tempere.h"
#include <cassert>
#include <algorithm>
#include <math.h>

using geom::Edge;
using geom::Vector;
using geom::Vertex;
using geom::Polygon;
using geom::vrtcomp;
using geom::edgecomp;
using opt::Optional;
using chain::ChainState;
using chain::PathState;
using chain::Chainshard;

/* Helper Functions */
Polygon vectorThunk(std::set<Vertex,vrtcomp> vec)
{
	Polygon ret = {};
	for(auto v : vec) { ret.push_back(v); }
	return ret;
}

std::vector<Vertex> sortInter(const Edge e, const Polygon base)
{
	// The sorting lambda(s), sort by distance to the head
	auto distance = [=](const Vertex &a) -> double
	{
		return geom::magnitude(geom::vec(e.head,a));
	};
	auto keylambda = [=](const Vertex &a, const Vertex &b) -> bool
	{
		return distance(a) > distance(b);
	};
	std::vector<Vertex> inter;
	for(auto b : geom::edgeThunk(base))
	{
		if(geom::eq(e,b)) { continue; }
		auto eb = geom::intersect_edge_edge(e,b);
		if(eb.is) { inter.push_back(eb.dat); }
	}
	bool addfront = inter.size() == 0 || !(geom::eq(inter.front(),e.head));
	bool addback = inter.size() == 0 || !(geom::eq(inter.back(),e.tail));
	if(addfront) { inter.insert(inter.begin(),e.head); }
	if(addback) { inter.push_back(e.tail); }
	std::sort(inter.begin(),inter.end(),keylambda);
	return inter;
}

/* Main Function Implementations */
uint32_t chain::Chainshard::ensureID(Vertex vrt)
{
	uint32_t id = node.size();
	auto idO = geom::find(node,vrt);
	if(idO.is) { return idO.dat; }
	node.push_back(vrt);
	graph[id] = {};
	return id;
}

void chain::Chainshard::shatter(const Polygon glass, const Polygon shard)
{
	// Degenerate edges have overlapp
	auto countedge = [](Polygon p, Edge edge) -> uint32_t
	{
		uint32_t ret = 0;
		for(auto e : edgeThunk(p)) { if(geom::eq(edge,e)) { ret++; } }
		return ret;
	};
	// Ensure nodes don't connect to duplicates or themselves
	auto addlambda = [=](uint32_t h, uint32_t t) -> void
	{
		if(h == t) { return; }
		graph[h].insert(node[t]);
	};
	auto lineshatter = [=](const Polygon base, const Polygon chisel) -> void
	{
		for(auto b : edgeThunk(base))
		{
			// Mark degenerate portions of the base
			if(countedge(base,b) > 1)
			{
				if(eq(b,{{5.401924,9.000000},{8.0,9.0}}))
				{
					printf("MULTIPLE COUNTS OF INVALID!\n");
				}
				mark.insert(b);
			}
			// For each edge get all intersection verticies, sorted
			auto sortBase = sortInter(b,chisel);
			// Now add them all to the graph
			for(uint32_t i = 0; i < (sortBase.size()-1); i++)
			{
				uint32_t hid = ensureID(sortBase[i]);
				uint32_t tid = ensureID(sortBase[i+1]);
				addlambda(hid,tid);
				addlambda(tid,hid);
			}
		}
	};
	lineshatter(glass,shard);
	lineshatter(shard,glass);
}

Optional<PathState> chain::stateDel(PathState S, Vertex next)
{
	// Just copy for now
	PathState ret = S;
	// Find next novel vertex
	// Optional<Vertex> prev = S.previous;
	// Check if a loop has occured, need full edge not just current vrt
	uint32_t pathlen = S.path.size();
	std::vector<Vertex> P = {};
	for(int tid = (pathlen-1); tid >= 0; tid--)
	{
		uint32_t hid = (pathlen + tid - 1) % pathlen;
		P.push_back(S.path[tid]);
		if(eq(S.current,S.path[tid]) && eq(S.previous,S.path[hid]))
		{
			// TODO: check if this case is truly superfluous
			// if(geom::eq(geom::signed_area(P),0.0)) { continue; }
			ret.action = chain::PathState::DONE;
			ret.path = P;
			return {true, ret};
		}
	}
	// By default move along the path
	ret.path.push_back(S.current);
	ret.previous = S.current;
	ret.current = next;
	return {true, ret};
}

Polygon chain::weave(const chain::ChainState current)
{
	auto inpoint = [=](Polygon poly) -> Optional<Vertex>
	{
		if(poly.size() == 0) { return { false, {0.0,0.0}}; }
		if(poly.size() < 3) { return { false, poly[0] }; }
		for(uint32_t i = 0; i < poly.size(); i++)
		{
			uint32_t j = (i + 1) % poly.size();
			uint32_t k = (i + 2) % poly.size();
			std::vector<Vertex> shard = {poly[i],poly[j],poly[k]};
			Vector v1 = geom::vec(shard[1],shard[0]);
			Vector v2 = geom::vec(shard[1],shard[2]);
			double angle = geom::angle(v1,v2);
			Vertex mid = geom::midpoint(shard);
			if(geom::eq(angle,0.0)) { continue; }
			if(geom::eq(angle,M_PI)) { continue; }
			// TODO: EFFICIENCY. Dead case?
			if(geom::find(poly,mid).is) { continue; }
			if(geom::interior(poly,mid)) { return { true, mid }; }
		}
		return { false, poly[0] };
	};
	// If the paths are the same we have a disconnected segment
	Polygon left = current.left.path;
	Polygon right = current.right.path;
	// Left handed turns match with positive (counterclockwise) rotation
	auto minpolycheck = [=](Polygon poly, int32_t wn) -> bool
	{
		auto optin = inpoint(poly);
		return optin.is && geom::winding_number(poly,optin.dat) == wn;
	};
	if(minpolycheck(left,1)) { return left; }
	if(minpolycheck(right,-1)) { return right; }
	printf("WEAVE ERROR!\n");
	// TODO: ERROR HANDLING. If there is no polygon found we fucked up
	printf("\n");
	printf("LEFT %d\n\t",geom::winding_number(left,inpoint(left).dat));
	for(auto l : left) { printf("(%f,%f) -- ",l.x,l.y); }
	printf("\n");
	printf("RIGHT %d\n\t",geom::winding_number(right,inpoint(right).dat));
	for(auto r : right) { printf("(%f,%f) -- ",r.x,r.y); }
	printf("\n");
	assert(false);
	return left;
}

const Optional<Edge> chain::Chainshard::nextUnmarked(
	std::set<Edge,edgecomp> mark)
{
	// Construct the next chain by getting a random unmarked edge
	Optional<Edge> base = { false, { {0.0,0.0}, {0.0,0.0} } };
	for(auto v : node)
	{
		for(auto c : graph[ensureID(v)])
		{
			Edge edge = {v,c};
			if(!mark.count(edge)) { return { true, { v, c } }; }
		}
	}
	return base;
}

const std::vector<Vertex> chain::Chainshard::getNode() { return node; }
const std::set<Edge,edgecomp> chain::Chainshard::fixedMark() { return mark; }

// TODO: we need to initialize the first path between two unmarked nodes
ChainState chain::initChainState(Edge e)
{
	ChainState ret;
	ret.left = { chain::PathState::RUN, {e.head}, e.tail, e.head };
	ret.right = { chain::PathState::RUN, {e.head}, e.tail, e.head };
	return ret;
}

const std::vector<Vertex> chain::Chainshard::sortedPath(Vertex vertex)
{
	return sortedPath(vertex,Optional<Vertex>{false,vertex});
}

const std::vector<Vertex> chain::Chainshard::sortedPath(Edge edge)
{
	// Our head vector is the edge, our tail the connection
	auto nid = geom::find(node,edge.tail);
	if(!nid.is) { return {}; }
	std::vector<Vertex> ret = {};
	for(auto g : graph[nid.dat])
	{
		if(geom::eq(g,edge.head)) { continue; }
		ret.push_back(g);
	};
	// If the only connection is degenerate
	if(ret.size() == 0) { return { edge.head }; }
	// A lambda to compare directed angles
	auto anglelambda = [=](Vertex v) -> double
	{
		return geom::dirangle(edge,v);
	};
	// A sortation by angle lambda
	auto sortlambda = [=](Vertex a, Vertex b) -> bool
	{
		return anglelambda(a) < anglelambda(b);
	};
	// Sort the return vector
	std::sort(ret.begin(),ret.end(),sortlambda);
	return ret;
}

const std::vector<Vertex> chain::Chainshard::sortedPath(
	Vertex v, Optional<Vertex> o)
{
	if(o.is) { return sortedPath(Edge{o.dat,v}); }
	return sortedPath(Edge{{1.0,0.0},v});
}

std::vector<Polygon> chain::chain(Chainshard* shard)
{
	// The return value
	std::vector<Polygon> ret = {};
	// A copy of the graph vertices
	const std::vector<Vertex> node = shard->getNode();
	if(node.size() == 0) { return {}; }
	if(node.size() < 3) { return { node }; }
	// Initialize the state for the chain algorithm
	auto mark = shard->fixedMark();
	if(mark.count({{5.401924,9.000000},{8.0,9.0}}))
	{
		printf("MARKED FROM BEGINNING!\n\t");
		for(auto m : mark)
		{
			printf("(%f,%f) -> (%f,%f)", m.head.x,m.head.y,m.tail.x,m.tail.y);
		}
		printf("\n");
	}
	// Basic loop: find the next polygon then mark enclosed vertices
	uint32_t b = 0;
	// Lambda to process a single path to completion
	auto runpath = [=](PathState P, chain::HANDEDNESS hand)
	{
		uint32_t breaker = 0;
		while(P.action == PathState::RUN && breaker < 100)
		{
			auto cand = shard->sortedPath({P.previous,P.current});
			bool chirality = hand == chain::HANDEDNESS::RIGHT;
			Vertex next = chirality ? cand.back() : cand.front();
			auto stateopt = stateDel(P,next);
			if(stateopt.is) { P = stateopt.dat; }
			breaker++;
		}
		return P;
	};
	// DEBUG SET
	//while(mark.size() < node.size() && b < 100)
	while(b < 100)
	{
		Optional<Edge> base = shard->nextUnmarked(mark);
		// If there is no unmarked edge we are done
		if(!base.is) { break; }
		// TODO: remove debug statement
		// Construct the next chain state
		ChainState S = initChainState(base.dat);
		// Run both paths to completion
		S.left = runpath(S.left,chain::HANDEDNESS::LEFT);
		S.right = runpath(S.right,chain::HANDEDNESS::RIGHT);
		// Then weave a polygon from the chain state
		// TODO: remove debug statement
		Polygon newpoly = weave(S);
		// Add the new polygon
		ret.push_back(newpoly);
		// Create new marks for this polygon
		for(auto u : edgeThunk(newpoly)) { mark.insert(u); }
		b++;
		if(b > 75) { printf("TOO MANY SEGS!"); assert(false); }
	}
	return ret;
}
