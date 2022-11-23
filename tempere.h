//C imports
#include <stdlib.h>
#include <stdio.h>

//C++ imports
#include <functional>
#include <map>

// Module imports
#include "geom.h"
#include "optional.h"

#ifndef TEMPERE_H
#define TEMPERE_H
using geom::Vertex;
using geom::Vector;
using geom::Edge;
using geom::Polygon;
using opt::Optional;

namespace chain
{
	class Chainshard
	{
		private:
			std::vector<Vertex> inter;
			std::map<uint32_t,uint32_t> interPlaceA;
			std::map<uint32_t,uint32_t> interPlaceB;
			std::map<uint32_t,std::vector<bool>> mark;
			std::map<uint32_t,std::vector<Edge>> path;
			Optional<uint32_t> minUnmarkedSlope(Vertex,uint32_t);
			void shatter(Polygon glass, Polygon shard);
		public:
			enum CROSS { INTER, SINK, NONE };
			struct Chainret
			{
				Edge path;
				Vertex inter;
				CROSS type;
			};
			Chainshard();
			Optional<Chainret> nextUnmarked();
			Chainret chainMark(Vertex);
			Chainshard(Polygon glass, Polygon shard)
			{
				shatter(glass, shard);
			}
		private:
			struct ChainshardID { uint32_t inter; uint32_t path; };
			Optional<struct ChainshardID> findpath(Vertex inter);
	};

	struct ChainState
	{
		struct Segment { Vertex inter; Polygon chain; };
		Vertex vrt;
		Vertex inter;
		Edge path;
		Chainshard::CROSS chi;
		uint32_t pA;
		uint32_t pB;
		std::vector<Edge> eA;
		std::vector<Edge> eB;
		std::vector<Segment> seg;
	};

	Optional<ChainState> sinkCase(ChainState,Vertex);
	Optional<ChainState> noneCase(ChainState);
	Optional<ChainState> interCase(ChainState,Edge,Vertex);


	ChainState initChainState(Polygon, Polygon);
	ChainState cleanChainState(ChainState, Chainshard::Chainret);
	Optional<ChainState> stateDel(ChainState, Chainshard::Chainret);
	std::vector<Polygon> weave(ChainState current, ChainState previous);
	std::vector<Polygon> chain(Polygon a, Polygon b, Chainshard* shard);
};
#endif
