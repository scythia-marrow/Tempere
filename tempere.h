//C imports
#include <stdlib.h>
#include <stdio.h>

//C++ imports
#include <functional>
#include <map>
#include <vector>
#include <set>

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
			std::vector<Vertex> node;
			std::map<uint32_t,std::set<Vertex,geom::vrtcomp>> graph;
			//Optional<uint32_t> minUnmarkedSlope(uint32_t);
			void shatter(const Polygon glass, const Polygon shard);
			uint32_t ensureID(Vertex);
		public:
			Chainshard();
			const std::vector<Vertex> getNode();
			const std::set<Vertex,geom::vrtcomp> fixedMark();
			// Find the next unmarked edge
			const Optional<Edge> nextUnmarked(
				std::set<Vertex,geom::vrtcomp>);
			// Find unique vertexes of a polygon
			const std::set<Vertex,geom::vrtcomp> unique(
				Polygon,std::set<Vertex,geom::vrtcomp>);
			// Sort a path just by angle
			const std::vector<Vertex> sortedPath(Vertex);
			// Sort a path by signed angle from an edge
			const std::vector<Vertex> sortedPath(Edge);
			// Sort a path by either, good for smoothing edge cases
			const std::vector<Vertex> sortedPath(
				Vertex,Optional<Vertex>);
			Chainshard(Polygon glass, Polygon shard)
			{
				shatter(glass, shard);
			}
	};

	struct PathState
	{
		enum ACTION { RUN, DONE, ERROR };
		ACTION action;
		std::vector<Vertex> path;
		Vertex current;
		Vertex previous;
	};

	struct ChainState
	{	
		//enum HANDEDNESS { LEFT, RIGHT };
		PathState left;
		PathState right;
	};

	enum HANDEDNESS { LEFT, RIGHT };
	ChainState initChainState(const Edge base);
	Optional<PathState> stateDel(const PathState, Vertex);
	Polygon weave(const ChainState);
	std::vector<Polygon> chain(Chainshard* shard);
};
#endif
