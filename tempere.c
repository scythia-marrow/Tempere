#include "geom.h"
#include "tempere.h"
#include <algorithm>

using geom::Edge;
using geom::Vector;
using geom::Vertex;
using geom::Polygon;
using opt::Optional;
using chain::ChainState;
using chain::PathState;
using chain::Chainshard;

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
	for(auto g : edgeThunk(glass))
	{
		// Get the head and tail of the intersection line segment
		uint32_t hid = ensureID(g.head);
		uint32_t tid = ensureID(g.tail);
		// The sorting lambda(s), sort by distance to the head
		auto distance = [=](uint32_t a) -> double
		{
			return geom::magnitude(geom::vec(g.head,node[a]));
		};
		auto keylambda = [=](uint32_t a, uint32_t b) -> bool
		{

			return distance(a) > distance(b);
		};
		// Ensure nodes don't connect to duplicates or themselves
		auto addlambda = [=](uint32_t h, uint32_t t) -> void
		{
			if(h == t) { return; }
			graph[h].insert(node[t]);
		};
		// For each edge get all intersection verticies, sorted
		std::vector<uint32_t> inter = {};
		for(auto s : edgeThunk(shard))
		{
			if(eq(g,s)) { continue; }
			auto gs = geom::intersect_edge_edge(g,s);
			if(gs.is)
			{
				uint32_t interID = ensureID(gs.dat);
				uint32_t shID = ensureID(s.head);
				uint32_t stID = ensureID(s.tail);
				inter.push_back(interID);
				if(shID == interID || stID == interID)
				{
					addlambda(shID,stID);
					addlambda(stID,shID);
				}
				else
				{
					addlambda(ensureID(s.head),interID);
					addlambda(ensureID(s.tail),interID);
				}
			}
		}
		std::sort(inter.begin(),inter.end(),keylambda);
		// Add the edge head and tail to the intersection nodes
		bool addfront = inter.size() == 0 || inter.front() != hid;
		bool addback = inter.size() == 0 || inter.back() != tid;
		if(addfront) { inter.insert(inter.begin(),hid); }
		if(addback) { inter.push_back(tid); }
		// Now add them all
		for(int i = 0; i < (inter.size()-1); i++)
		{
			addlambda(inter[i],inter[i+1]);
			addlambda(inter[i+1],inter[i]);
		}
	}
	// Print the graph
	printf("NODE\n");
	for(int i = 0; i < node.size(); i++)
	{
		printf("\t(%f,%f)\n\t\t",node[i].x,node[i].y);
		for(auto n : graph[i])
		{
			printf("(%f,%f) -- ",n.x,n.y);
		}
		printf("\n");
	}
}

Optional<PathState> chain::stateDel(PathState S, Vertex next)
{
	// Just copy for now
	PathState ret = S;
	// Find next novel vertex
	Optional<Vertex> prev = S.previous;
	// Check if a loop has occured, need full edge not just current vrt
	uint32_t pathlen = S.path.size();
	std::vector<Vertex> P = {};
	for(int tid = (pathlen-1); tid >= 0; tid--)
	{
		if(!S.previous.is) { break; }
		uint32_t hid = (pathlen + tid - 1) % pathlen;
		P.push_back(S.path[tid]);
		if(eq(S.current,S.path[tid]) && eq(S.previous.dat,S.path[hid]))
		{
			if(geom::eq(geom::signed_area(P),0.0)) { continue; }
			ret.action = chain::PathState::DONE;
			ret.path = P;
			return {true, ret};
		}
	}
	// By default move along the path
	ret.path.push_back(S.current);
	ret.previous = {true, S.current};
	ret.current = next;
	return {true, ret};
}

Polygon chain::weave(const chain::ChainState current)
{
	auto inpoint = [=](Polygon poly) -> Vertex
	{
		if(poly.size() == 0) { return {0.0,0.0}; }
		if(poly.size() < 3) { return poly[0]; }
		for(int i = 0; i < poly.size(); i++)
		{
			uint32_t j = (i + 1) % poly.size();
			uint32_t k = (i + 2) % poly.size();
			std::vector<Vertex> shard = {poly[i],poly[j],poly[k]};
			Vertex midpoint = geom::midpoint(shard);
			if(geom::interior(midpoint,poly)) { return midpoint; }
		}
		return { poly[0] };
	};
	Polygon left = current.left.path;
	Polygon right = current.right.path;
	printf("WEAVE\n\t");
	printf("LEFT (%f,%f) %d\n\t\t",inpoint(left).x,inpoint(left).y,geom::winding_number(inpoint(left),left));
	for(auto v : left)
	{
		printf("(%f,%f) -- ",v.x,v.y);
	}
	printf("\n\t");
	printf("RIGHT (%f,%f) %d\n\t\t",inpoint(right).x,inpoint(right).y,geom::winding_number(inpoint(right),right));
	for(auto v : right)
	{
		printf("(%f,%f) -- ",v.x,v.y);
	}
	printf("\n");
	// Left handed turns match with positive (counterclockwise) rotation
	if(geom::winding_number(inpoint(left),left) == 1) { return left; }
	if(geom::winding_number(inpoint(right),right) == -1) { return right; }
	printf("ERROR!\n");
	// TODO: ERROR HANDLING. If there is no polygon found we fucked up
	Polygon ret = {};
	return ret;
}

Optional<Vertex> chain::nextUnmarked(const Polygon node, const Polygon mark)
{
	// Construct the next chain by getting a random initial vertex
	Optional<Vertex> base = { false, {0.0,0.0} };
	for(auto v : node) { if(!geom::find(mark,v).is) { base = {true, v}; } }
	return base;
}

const std::vector<Vertex> chain::Chainshard::getNode() { return node; }

ChainState chain::initChainState(Vertex vrt, std::vector<Vertex> mark)
{
	ChainState ret;
	ret.left = { chain::PathState::RUN, {}, vrt, {false,vrt} };
	ret.right = { chain::PathState::RUN, {}, vrt, {false,vrt} };
	ret.mark = {};
	return ret;
}

const std::vector<Vertex> chain::Chainshard::sortedPath(Vertex vertex)
{
	auto nid = geom::find(node,vertex);
	if(!nid.is) { return {}; }
	std::vector<Vertex> ret = {};
	for(auto g : graph[nid.dat]) { ret.push_back(g); };
	// A lambda to compare raw angles
	auto anglelambda = [=](Vertex v) -> double
	{
		return geom::angle(vertex,v);
	};
	// A sortation by raw angles
	auto sortlambda = [=](Vertex a, Vertex b) -> bool
	{
		return anglelambda(a) > anglelambda(b);
	};
	// Sort the return vector
	std::sort(ret.begin(),ret.end(),sortlambda);
	return ret;
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
	// Print the sort and angle
	if(ret.size() > 2)
	{
		printf("SORTED (%f,%f) -> (%f,%f)\n\t",edge.head.x,edge.head.y,edge.tail.x,edge.tail.y);
		for(auto n : ret)
		{
			printf("(%f%f) %f -- ",n.x,n.y,anglelambda(n));
		}
		printf("\n");
	}
	
	return ret;
}

const std::vector<Vertex> chain::Chainshard::sortedPath(
	Vertex v, Optional<Vertex> o)
{
	if(o.is) { return sortedPath(Edge{o.dat,v}); }
	return sortedPath(v);
}

Polygon vectorThunk(std::set<Vertex,geom::vrtcomp> vec)
{
	Polygon ret = {};
	for(auto v : vec) { ret.push_back(v); }
	return ret;
}

std::vector<Polygon> chain::chain(Chainshard* shard)
{
	// The return value
	std::vector<Polygon> ret = {};
	// A copy of the graph vertices
	const std::vector<Vertex> node = shard->getNode();
	if(node.size() == 0) { return {}; }
	// Initialize the state for the chain algorithm
	std::set<Vertex,geom::vrtcomp> mark = {};
	// Basic loop: find the next polygon then mark enclosed vertices
	uint32_t b = 0;
	// Lambda to process a single path to completion
	auto runpath = [=](PathState P, ChainState::HANDEDNESS hand)
	{
		while(P.action == PathState::RUN)
		{
			auto cand = shard->sortedPath(P.current,P.previous);
			bool chirality = hand == ChainState::HANDEDNESS::RIGHT;
			if(!P.previous.is) { chirality = true; }
			Vertex next = chirality ? cand.back() : cand.front();
			auto stateopt = stateDel(P,next);
			if(stateopt.is) { P = stateopt.dat; }
		}
		return P;
	};
	while(mark.size() < node.size() && b < 10)
	{
		Optional<Vertex> base = nextUnmarked(node,vectorThunk(mark));
		// Error if there is none, should never happen
		if(!base.is) { printf("IMPOSSIBLE SITUATION\n"); return ret; }
		printf("BASE (%f%f)\n",base.dat.x,base.dat.y);
		// Construct the next chain state
		ChainState S = initChainState(base.dat, {});
		// Run both paths to completion
		S.left = runpath(S.left,ChainState::HANDEDNESS::LEFT);
		S.right = runpath(S.right,ChainState::HANDEDNESS::RIGHT);
		// Then weave a polygon from the chain state
		Polygon newpoly = weave(S);
		ret.push_back(newpoly);
		// Create new marks for this polygon
		for(auto v : newpoly) { mark.insert(v); }
		b++;
	}
	return ret;
}
