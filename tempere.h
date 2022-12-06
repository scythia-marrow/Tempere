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
		/*public:
			struct setcomp {
			bool operator() (const Vertex &a,const Vertex &b) const
			{
				Vector vec = geom::vec(a,b);
				if(geom::magnitude(vec) < geom::EPS)
				{
					return false;
				}
				return geom::magnitude(a) < geom::magnitude(b); 
			}
			};	
		*/
		private:
			std::vector<Vertex> node;
			std::map<uint32_t,std::set<Vertex,geom::vrtcomp>> graph;
			//Optional<uint32_t> minUnmarkedSlope(uint32_t);
			void shatter(const Polygon glass, const Polygon shard);
			uint32_t ensureID(Vertex);
		public:
			Chainshard();
			const std::vector<Vertex> getNode();
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
		Optional<Vertex> previous;
	};

	struct ChainState
	{	
		enum HANDEDNESS { LEFT, RIGHT };
		PathState left;
		PathState right;
		std::vector<Vertex> mark;
	};

	enum HANDEDNESS { LEFT, RIGHT };
	ChainState initChainState(const Vertex base, const Polygon mark);
	Optional<Vertex> nextUnmarked(const Polygon node, const Polygon mark);
	Optional<PathState> stateDel(const PathState, Vertex);
	Polygon weave(const ChainState);
	std::vector<Polygon> chain(Chainshard* shard);
};
#endif
