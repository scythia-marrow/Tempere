#include "geom.h"
#include "tempere.h"
#include <algorithm>

using geom::Edge;
using geom::Vector;
using geom::Vertex;
using geom::Polygon;
using opt::Optional;
using chain::ChainState;
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

Optional<ChainState> chain::stateDel(ChainState S, std::vector<Vertex> cand)
{
	printf("STATE DEL! (%f,%f) -- (%f,%f)\n\t",S.previous.dat.x,S.previous.dat.y,S.current.x,S.current.y);
	printf("S\n\t\t");
	for(auto n : S.path)
	{
		printf("(%f,%f) --",n.x,n.y);
	}
	printf("\n\tCAND %d\n\t\t",cand.size());
	for(auto i : cand)
	{
		printf("(%f,%f) --",i.x,i.y);
	}
	printf("\n");
	// Just copy for now
	ChainState ret = S;
	// Find next novel vertex
	Optional<Vertex> prev = S.previous;
	Optional<Vertex> next = {false, {0.0,0.0}};
	for(auto v : cand)
	{
		if(!geom::find(ret.mark,v).is)
		{
			// Do not move backwards lol
			if(prev.is && geom::eq(prev.dat,v)) { continue; }
			next = {true, v};
			// Take the smallest next vertex
			break;
		}
	}
	// If there is no next unmarked set an error
	if(!next.is) { ret.action = chain::ChainState::ERROR; }
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
			ret.action = chain::ChainState::DONE;
			ret.path = P;
			return {true, ret};
		}
	}
	// By default move along the path
	ret.path.push_back(S.current);
	ret.previous = {true, S.current};
	ret.current = next.dat;
	return {true, ret};
}

Polygon chain::weave(const chain::ChainState current)
{
	printf("WEAVING!\n\t");
	for(auto n : current.path)
	{
		printf("(%f,%f) --",n.x,n.y);
	}
	// TODO: better thingy than this
	Polygon ret = current.path;
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
	return { chain::ChainState::ACTION::RUN, mark, {}, vrt, {false,vrt} };
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
	for(auto g : graph[nid.dat]) { ret.push_back(g); };
	// A lambda to compare directed angles
	auto anglelambda = [=](Vertex v) -> double
	{
		Vector head = geom::vec(edge.head,edge.tail);
		Vector tail = geom::vec(edge.tail,v);
		return geom::dirangle(head,tail);
	};
	// A sortation by angle lambda
	auto sortlambda = [=](Vertex a, Vertex b) -> bool
	{
		return anglelambda(a) > anglelambda(b);
	};
	// Sort the return vector
	std::sort(ret.begin(),ret.end(),sortlambda);
	// Print it
	if(ret.size() > 2)
	{
		printf("SORTED (%f,%f)--(%f,%f)\n\t",edge.head.x,edge.head.y,edge.tail.x,edge.tail.y);
		for(auto x : ret) { printf("%f%f -- ",x.x,x.y); }
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

std::vector<Polygon> chain::chain(Chainshard* shard)
{
	// The return value
	std::vector<Polygon> ret = {};
	// A copy of the graph vertices
	const std::vector<Vertex> node = shard->getNode();
	if(node.size() == 0) { return {}; }
	// Initialize the state for the chain algorithm
	std::vector<Vertex> mark = {};
	// Basic loop: find the next polygon then mark enclosed vertices
	while(mark.size() < node.size())
	{
		Optional<Vertex> base = nextUnmarked(node,mark);
		// Error if there is none, should never happen
		if(!base.is) { printf("IMPOSSIBLE SITUATION\n"); return ret; }
		// Construct the next chain state
		ChainState S = initChainState(base.dat, mark);
		while(S.action == ChainState::RUN)
		{
			auto cand = shard->sortedPath(S.current,S.previous);
			auto stateopt = stateDel(S,cand);
			// Again, should never happen
			if(!stateopt.is) { break; }
			S = stateopt.dat;
		}
		// At the end do cleanup
		if(S.action == ChainState::ERROR)
		{
			printf("CHAIN ERROR!\n");
			return ret;
		}
		Polygon newpoly = weave(S);
		ret.push_back(newpoly);
		// Create new marks for each vertex with no free connections
		for(auto v : newpoly)
		{
			auto connect = shard->sortedPath(v);
			Optional<Vertex> num = nextUnmarked(connect,newpoly);
			if(!num.is) { mark.push_back(v); }
		}
	}
	return ret;
}
