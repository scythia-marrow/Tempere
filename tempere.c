#include "geom.h"
#include "tempere.h"

using geom::Edge;
using geom::Vector;
using geom::Vertex;
using geom::Polygon;
using opt::Optional;
using chain::ChainState;
using chain::Chainshard;

// Find the index of an edge closest along the chain to a given intersection
Optional<uint32_t> edgeFind(std::vector<Edge> poly, Vertex v, uint32_t strt)
{
	for(int i = 0; i < poly.size(); i++)
	{
		uint32_t place = (strt+i) % poly.size();
		Edge p = poly[place];
		if(eq(v,p.head)) { return {true,place}; }
	}
	return {false,0};
}

void printSeg(ChainState state)
{
	printf("State:\n\t");
	for(auto seg : state.seg)
	{
		printf("segment (%fx%f)\n\t",seg.inter.x,seg.inter.y);
		for(auto ch : seg.chain)
		{
			printf("%f,%f -> ",ch.x,ch.y);
		}
		printf("\n\t");
	}
	printf("\n");
}

// None case, follow along a chain
Optional<ChainState> chain::noneCase(ChainState state)
{
	ChainState ret = state;
	// If we moved along a chain
	Vertex next;
	bool moved = false;
	std::function<uint32_t(std::vector<Edge>,uint32_t)> link =
	[&](std::vector<Edge> poly, uint32_t start) -> uint32_t
	{
		auto place = edgeFind(poly,state.vrt,start);
		if(!place.is) { return 0; }
		Edge p = poly[place.dat];
		next = p.tail;
		moved = true;
		return place.dat;
	};
	bool anymove = false;
	uint32_t alongA = link(state.eA, state.pA);
	if(moved) { anymove = true; ret.vrt = next; }
	uint32_t alongB = link(state.eB, state.pB);
	if(moved) { anymove = true; ret.vrt = next; }
	if(anymove)
	{
		state.pA = alongA;
		state.pB = alongB;
		ret.seg.back().chain.push_back(state.vrt);
	}
	printSeg(ret);
	return {moved, ret};
}

// Intersection case, move along the tightest unmarked path
Optional<ChainState> chain::interCase(ChainState S, Edge path, Vertex inter)
{
	ChainState ret = S;
	auto pA = edgeFind(S.eA,S.vrt,S.pA);
	auto pB = edgeFind(S.eB,S.vrt,S.pB);
	printf("S INTER:  %f%f\n",S.inter.x,S.inter.y);
	printf("N INTER:  %f%f\n",inter.x,inter.y);
	printf("P HEAD: %f%f\n",path.head.x,path.head.y);
	printf("P TAIL: %f%f\n",path.tail.x,path.tail.y);
	printf("VRT: %f%f\n",S.vrt.x,S.vrt.y);
	// Add a new segment to the chain
	ret.seg.back().chain.push_back(S.vrt);
	ret.seg.push_back(ChainState::Segment{inter, Polygon{}});
	// Degenerate case, move immediately
	// Update state items to move forward
	if(pA.is) { ret.pA = pA.dat; }
	if(pB.is) { ret.pB = pB.dat; }
	ret.vrt = inter;
	ret.inter = inter;
	printSeg(ret);
	return {true, ret};
}

// Sink case
Optional<ChainState> chain::sinkCase(ChainState state)
{
	return {false, state};
}

Optional<Chainshard::ChainshardID> chain::Chainshard::findpath(Vertex vrt)
{
	for(int i = 0; i < inter.size(); i++)
	{
		for(int p = 0; p < path[i].size(); p++)
		{
			if(eq(vrt,path[i][p].head))
			{
				return {true, {i,p}};
			}
		}
	}
	return {false,0};
}

Optional<Chainshard::Chainret> chain::Chainshard::nextUnmarked()
{
	Chainret ret;
	for(uint32_t i = 0; i < inter.size(); i++)
	{
		auto mum = minUnmarkedSlope(inter[i], i);
		if(mum.is)
		{
			ret.path = path[i][mum.dat];
			ret.inter = inter[i];
			ret.type = Chainshard::CROSS::NONE;
			return {true, ret};
		}
	}
	return {false, ret};
}

Chainshard::Chainret chain::Chainshard::chainMark(Vertex vrt)
{
	auto idx = chain::Chainshard::findpath(vrt);
	Vertex zero {0.0,0.0};
	if(!idx.is) { return {{zero,zero}, vrt, Chainshard::CROSS::NONE}; }
	// Pull intersection data
	Chainshard::ChainshardID csid = idx.dat;
	Vertex interV = inter[csid.inter];
	// This is a sink if there are no unmarked left
	Optional<uint32_t> mUMS = minUnmarkedSlope(vrt, csid.inter);
	// On a sink return the intersection vector
	if(!mUMS.is) { return {{zero,zero}, interV, Chainshard::CROSS::SINK}; }
	// Otherwise return the next path we wish to follow, and mark it
	Edge pathE = path[csid.inter][mUMS.dat];
	mark[csid.inter][mUMS.dat] = true;
	// If the intersection is not degenerate
	return { pathE, interV, Chainshard::CROSS::INTER };
}
	
void chain::Chainshard::shatter(Polygon glass, Polygon shard)
{
	// Add a path/ori/mark triple
	auto addlambda = [=](uint32_t i, Vertex v, Edge edge) mutable -> void
	{
		// Only add unique paths with interior direction
		if(geom::find(path[i],edge).is || eq(edge.tail,v)) { return; }
		path[i].push_back(edge);
		mark[i].push_back(false);
	};
	// Store all edge intersections, associate them with edges
	auto glassEdge = edgeThunk(glass);
	auto shardEdge = edgeThunk(shard);
	for(int ge = 0; ge < glassEdge.size(); ge++)
	{
		Edge eg = glassEdge[ge];
		for(int se = 0; se < shardEdge.size(); se++)
		{
			Edge es = shardEdge[se];
			if(eq(eg,es)) { continue; }
			Optional<Vertex> interO = intersect_edge_edge(eg, es);
			if(!interO.is) { continue; }
			Vertex intV = interO.dat;
			Optional<uint32_t> idxO = geom::find(inter,intV);
			uint32_t idx;
			if(idxO.is) { idx = idxO.dat; }
			else
			{
				idx = inter.size();
				inter.push_back(intV);
				path[idx] = {};
				mark[idx] = {};
				interPlaceA[idx] = ge;
				interPlaceB[idx] = se;
			}
			// We do not want to add overlapping stuff
			addlambda(idx,intV,eg);
			addlambda(idx,intV,es);
		}
	}
	printf("INTER\n");
	for(int i = 0; i < inter.size(); i++)
	{
		printf("Inter (%f,%f)\n",inter[i].x,inter[i].y);
		for(auto p : path[i])
		{
			printf("\t%f,%f -- %f,%f\n",
				p.head.x,p.head.y,p.tail.x,p.tail.y);
		}
	}
	printf("\n");
}

Optional<uint32_t> chain::Chainshard::minUnmarkedSlope(Vertex v, uint32_t idx)
{
	double min;
	Optional<uint32_t> minO = {false, 0};
	for(int i = 0; i < path[idx].size(); i++)
	{
		printf("Mark %f%f %s\n",path[idx][i].head.x,path[idx][i].head.y,mark[idx][i] ? "true" : "false");
		if(mark[idx][i]) { continue; }
		Edge p = path[idx][i];
		double S = geom::dirangle(p.head,v);
		if(!minO.is || S < min)
		{
			minO = {true, i};
			min = S;
		}
	}
	return minO;
}

ChainState chain::initChainState(Polygon a, Polygon b)
{
	ChainState ret;
	ret.eA = edgeThunk(a);
	ret.eB = edgeThunk(b);
	return ret;
}

ChainState chain::cleanChainState(ChainState prev, Chainshard::Chainret um)
{
	Vertex vrt = um.path.tail;
	Vertex inter = um.inter;
	Chainshard::CROSS chirality = um.type;
	std::vector<ChainState::Segment> segment = 
		{ChainState::Segment{inter,Polygon{}}};
	return {vrt,inter,um.path,chirality,0,0,prev.eA,prev.eB,segment};
}

Optional<ChainState> chain::stateDel(ChainState state, Chainshard::Chainret n)
{
	// Update the state by placing the right path, ect.
	switch(n.type)
	{
		case Chainshard::CROSS::SINK:
			printf("SINK FOUND!\n");
			return sinkCase(state);
		case Chainshard::CROSS::NONE:
			printf("NONE FOUND\n");
			return noneCase(state);
		case Chainshard::CROSS::INTER:
			printf("INTER FOUND\n");
			return interCase(state,n.path,n.inter);
	}
}

Polygon chain::weave(ChainState next, ChainState prev)
{
	return {};
}

std::vector<Polygon> chain::chain(Polygon a, Polygon b, Chainshard* shard)
{
	// Initialize the state for the chain algorithm
	ChainState S = initChainState(a,b);
	std::vector<Polygon> ret;
	Optional<Chainshard::Chainret> um = shard->nextUnmarked();
	while(um.is)
	{
		S = cleanChainState(S, um.dat);
		// Path through the polygon
		int thing = 0;
		while(S.seg.size() > 0 && thing < 10)
		{
			// Start at the source, move and store loops
			auto next = shard->chainMark(S.vrt);
			// Update the state with different actions
			auto stateChange = stateDel(S,next);
			// If there is no change there is an error
			if(!stateChange.is) { return {}; }
			// If there is a sink, weave a polygon
			if(next.type == Chainshard::CROSS::SINK)
			{
				auto poly = weave(stateChange.dat, S);
				ret.push_back(poly);
			}
			// Update the state
			S = stateChange.dat;
			thing++;
		}
		break;
	}
	return {};
}
