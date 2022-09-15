// C imports
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// C++ imports
#include <map>

#include "geom.h"
using namespace geom;

Vertex geom::add(Vertex a, Vertex b)
{
	return {a.x + b.x, a.y + b.y};
}

Vector geom::sub(Vector a, Vector b)
{
	return {a.x - b.x, a.y - b.y};
}

Vertex geom::scale(Vertex a, double scale)
{
	return {a.x * scale, a.y * scale};
}

Vertex geom::normalize(Vertex a)
{
	Vertex origin {0.0, 0.0};
	double len = arclen((Edge){origin, a});
	return scale(a, 1.0 / len);
}

Vector geom::vec(Vertex a, Vertex b)
{
	return {b.x - a.x, b.y - a.y};
}

bool geom::eq(Vertex a, Vertex b)
{
	return eq(a,b,0.01);
}

bool geom::eq(Vertex a, Vertex b, double eps)
{
	bool eqx = a.x > b.x ? ((a.x - b.x) < eps) : ((b.x - a.x) < eps);
	bool eqy = a.y > b.y ? ((a.y - b.y) < eps) : ((b.y - a.y) < eps);
	return eqx && eqy;
}

bool geom::eq(Edge e1, Edge e2)
{
	if(eq(e1.head,e2.head) && eq(e1.tail, e2.tail)) { return true; }
	if(eq(e1.tail,e2.head) && eq(e1.head, e2.tail)) { return true; }
	return false;
}

double geom::magnitude(Vector a)
{
	return sqrt(a.x * a.x + a.y * a.y);
}

double geom::slope(Edge edge)
{
	Vector dir = sub(edge.tail,edge.head);
	return dir.y / dir.x;
}

double geom::angle(Vector a, Vector b)
{
	double mag_sum = magnitude(a) * magnitude(b);
	double dot_sum = dot(a,b);
	double quot = dot_sum / mag_sum;
	return acos(quot);
}

double geom::cross(Vector a, Vector b)
{
	return a.x * b.y - a.y * b.x;
}

double geom::dot(Vector a, Vector b)
{
	return a.x * b.x + a.y * b.y;
}

double geom::arclen(Edge e)
{
	return magnitude(vec(e.head,e.tail));
}

std::vector<Edge> geom::edgeThunk(Polygon boundary)
{
	std::vector<Edge> ret;
	for(int h = 0; h < boundary.size(); h++)
	{
		int t = (h + boundary.size() + 1) % boundary.size();
		ret.push_back(Edge{boundary[h],boundary[t]});
	}
	return ret;
}

Polygon geom::polygonThunk(std::vector<Edge> edge)
{
	Polygon ret;
	for(auto e : edge) { ret.push_back(e.head); }
	return ret;
}

double geom::perimeter(Polygon poly)
{
	double perim = 0.0;
	for(auto edge : edgeThunk(poly)) { perim += arclen(edge); }
	return perim;
	//double perim = arclen((Edge){vertexes[0], vertexes[1]});
	//perim += arclen((Edge){vertexes[vertexes.size() - 1], vertexes[0]});
	//for(unsigned int v = 1; v < vertexes.size() - 1; v++)
	//{
	//	perim += arclen((Edge){vertexes[v], vertexes[v + 1]});
	//}
	//return perim;
}

double geom::signed_area(Polygon poly)
{
	double area_s = 0.0;
	for(auto e : edgeThunk(poly)) { area_s += cross(e.head,e.tail); }
	return area_s / 2.0;
	//double area_s = 0.0;
	//int size = point.size();
	//for(int h = 0; h < size; h++)
	//{
	//	int t = (size + h + 1) % size;
	//	Vertex head = point[h];
	//	Vertex tail = point[t];
	//	area_s += cross(head, tail);
	//}
	//return area_s / 2.0;
}

Vertex geom::midpoint(std::vector<Vertex> cloud)
{
	Vertex vec {0.0, 0.0};
	for(auto point : cloud)
	{
		vec = add(vec, point);
	}
	vec = scale(vec, (1.0 / cloud.size()));
	return vec;
}

Vertex geom::centroid(Polygon poly)
{
	Vertex centroid {0.0 , 0.0};
	double area_s = signed_area(poly);
	for(auto e : edgeThunk(poly))
	{
		double a = cross(e.head, e.tail);
		centroid.x += (e.head.x + e.tail.x) * a;
		centroid.y += (e.head.y + e.tail.y) * a;
	}

	centroid.x /= (6.0 * area_s);
	centroid.y /= (6.0 * area_s);

	//printf("Centroid: (%f,%f)\n", centroid.x, centroid.y);
	return centroid;
}

Optional<Vertex> geom::intersect_ray_line(Vertex origin, Vector dir, Edge e)
{
	Vector p1 = add(origin, scale(e.head, -1.0));
	Vector p2 = add(e.tail, scale(e.head, -1.0));
	Vector p3 {-1.0f * dir.y, dir.x};

	double D = dot(p2,p3);
	if(abs(D) < 0.00001) { return {false, Vertex{0.0,0.0}}; }
	
	double t1 = cross(p2, p1) / D;
	double t2 = dot(p1,p3) / D;

	if (t1 >= 0.0 && (t2 >= 0.0 && t2 <= 1.0))
	{
		return {true, add(origin, (Vertex)scale(dir, t1))};
	}
	return {false,Vertex{0.0,0.0}};
}

Optional<Vertex> geom::intersect_ray_line(
	Vertex o, Vector dir,
	Vertex v1, Vertex v2)
{
	return intersect_ray_line(o, dir, Edge{v1,v2});
}

std::vector<Vertex> geom::intersect_ray_poly(Vertex o, Vector dir, Polygon poly)
{
	std::vector<Vertex> ret;
	for(auto e : edgeThunk(poly))
	{
		Optional<Vertex> vrt = intersect_ray_line(o, dir, e);
		// TODO: why is this here?
		if(!vrt.is) { continue; }
		ret.push_back(vrt.dat);
	}
	return ret;
}

Optional<Vertex> geom::intersect_edge_edge(Edge head, Edge tail)
{
	Vector ray = sub(head.tail, head.head);
	Optional<Vertex> interO = intersect_ray_line(head.head, ray, tail);
	// Check if the intersection is within the edge
	if(!interO.is) { return interO; }
	Vector interRay = vec(head.head,interO.dat);
	if(magnitude(interRay) > magnitude(ray)) { interO.is = false; }
	return interO;
}

Vertex geom::nearest_point(Vertex o, Polygon vtx)
{
	Vertex ret = o;
	double dis = -1.0;
	for(auto v : vtx)
	{
		double nrm = arclen((Edge){o,v});
		dis = dis == -1.0 || nrm < dis ? nrm : dis;
	}
	return ret;
}

Vertex geom::furthest_point(Vertex o, Polygon vtx)
{
	Vertex ret = o;
	double dis = -1.0;
	for(auto v : vtx)
	{
		double nrm = arclen((Edge){o,v});
		dis = dis == -1.0 || nrm > dis ? nrm : dis;
	}
	return ret;
}

bool geom::interior(Vertex v, Polygon poly)
{
	bool ret = winding_number(v, poly) != 0;
	return ret;
}

bool geom::interior(Polygon in, Polygon out)
{
	// Interior if all points are interior and no edge intersections
	printf("Checking interior...");
	for(auto vrt : in)
	{
		if(!interior(vrt,out))
		{
			printf("(%f,%f) not interior\n",vrt.x,vrt.y);
			return false;
		}
	}
	printf(" all interior\n");
	printf("Checking edge intersection(s)...");
	for(auto ie : edgeThunk(in))
	{
		for(auto oe : edgeThunk(out))
		{
			Optional<Vertex> interO = intersect_edge_edge(ie, oe);
			if(interO.is)
			{
				Vertex inter = interO.dat;
				printf(" found (%fx%f)\n", inter.x,inter.y);
				return false;
			}
		}
	}
	printf(" none found\n");
	return true;
}

uint32_t geom::winding_number(Vertex v, Polygon poly)
{
	auto is_left = [](Vertex p0, Vertex h0, Vertex t0)
	{
		return (((h0.x - p0.x) * (t0.y - p0.y))
			- ((t0.x - p0.x) * (h0.y - p0.y)));
	};
	uint32_t wn = 0;
	for(auto e : edgeThunk(poly))
	{
		Vertex H = e.head;
		Vertex T = e.tail;
		double left = is_left(H, T, v);
		bool wnplus = (H.y <= v.y) && (T.y >= v.y) && (left >= 0.0);
		bool wnminus = (H.y >= v.y) && (T.y <= v.y) && (left < 0.0);
		/*printf("WNbools %b,%b\n",wnplus,wnminus);
		printf("\t%b,%b,%b,IsLeft: %f\n", H.y <= v.y, T.y > v.y, left);
		printf("\tH,T -- p: (%f,%f), (%f,%f), (%f,%f)\n",
			H.x, H.y, T.x, T.y, v.x, v.y);
		*/
		if(wnplus && wnminus) { printf("PLUS AND MINUS! ON EDGE!\n"); }
		if(wnplus) { wn++; }
		if(wnminus) { wn--; }
	}
	return wn;
}

Optional<uint32_t> geom::find(Polygon poly, Vertex vert)
{
	for(int i = 0; i < poly.size(); i++)
	{
		if(eq(poly[i], vert)) { return {true,i}; }
	}
	return {false,0};
}


namespace chain
{
	class Chainshard
	{
		private:
			std::vector<Vertex> inter;
			std::map<uint32_t,std::vector<bool>> mark;
			std::map<uint32_t,std::vector<bool>> ori;
			std::map<uint32_t,std::vector<Vertex>> path;
			Optional<uint32_t> minUnmarkedSlope(uint32_t);
			void shatter(Polygon glass, Polygon shard);
			struct ChainshardID { uint32_t inter; uint32_t path; };
		public:
			enum CROSS { HEAD, TAIL, SINK, NONE };
			struct Chainret
			{
				Vertex path;
				Vertex inter;
				CROSS type;
			};
			Chainshard();
			Optional<ChainshardID> find(Vertex, CROSS);
			Optional<Chainret> nextUnmarked();
			Chainret chainMark(Vertex, CROSS);
			Chainshard(Polygon glass, Polygon shard)
			{
				shatter(glass, shard);
			}
	};

	Optional<uint32_t> chain::Chainshard::minUnmarkedSlope(uint32_t idx)
	{
		double min; 
		Optional<uint32_t> minO = {false, 0};
		for(int i = 0; i < path[idx].size(); i++)
		{
			if(mark[idx][i]) { continue; }
			Vertex p = path[idx][i];
			double S = slope(Edge{p,inter[idx]});
			if(!minO.is || S < min)
			{
				minO = {true, i};
			}
		}
		return minO;
	}

	Optional<chain::Chainshard::ChainshardID>
		chain::Chainshard::find(Vertex vrt, chain::Chainshard::CROSS P)
	{
		for(int i = 0; i < inter.size(); i++)
		{
			Optional<uint32_t> pid = geom::find(path[i],vrt);
			if(pid.is && ori[i][pid.dat] == P)
			{
				return {true, {i,pid.dat}};
			}
		}
		return {false,{0,0}};
	}

	Optional<chain::Chainshard::Chainret> chain::Chainshard::nextUnmarked()
	{
		Chainret ret;
		for(uint32_t i = 0; i < inter.size(); i++)
		{
			for(auto m : mark[i])
			{
				bool p = ori[i][m];
				CROSS H = CROSS::HEAD;
				CROSS T = CROSS::TAIL;
				CROSS chirality = ori[i][m] ? H : T;
					if(!m)
				{
				ret.path = path[i][m];
					ret.inter = inter[i];
					ret.type = chirality;
					return {true, ret};
				}
			}
		}
		return {false, ret};
	}

	chain::Chainshard::Chainret
		chain::Chainshard::chainMark(
			Vertex vrt, chain::Chainshard::CROSS chi)
	{
		auto idx = chain::Chainshard::find(vrt,chi);
		Vertex zero {0.0,0.0};
		if(!idx.is) { return {zero, zero, CROSS::NONE}; }
		ChainshardID csid = idx.dat;
		Vertex interV = inter[csid.inter];
		// Mark the vertex found
		mark[csid.inter][csid.path] = true;
		// This is a sink if there are no unmarked left
		Optional<uint32_t> mUMS = minUnmarkedSlope(csid.inter);
		// On a sink return the intersection vector
		if(!mUMS.is) { return {vrt, interV, CROSS::SINK}; }
		// Return the correct path, indicating head or tail
		Vertex pathV = path[csid.inter][mUMS.dat];
		bool pr = ori[csid.inter][mUMS.dat];
		return {pathV, interV, pr ? CROSS::HEAD : CROSS::TAIL};
	}
	
	void chain::Chainshard::shatter(Polygon glass, Polygon shard)
	{
		printf("Found %i edges\n",inter.size());
		// TODO: check if the polygons are equivalent when none found
		// Store all edge intersections, associate them with edges
		for(auto eg : edgeThunk(glass))
		{
			for(auto es : edgeThunk(shard))
			{
				if(eq(eg,es)) { continue; }
				Optional<Vertex> interO =
					intersect_edge_edge(eg, es);
				if(!interO.is) { continue; }
				Vertex interV = interO.dat;
				Optional<uint32_t> idxO =
					geom::find(inter,interV);
				uint32_t idx;
				if(idxO.is) { idx = idxO.dat; }
				else
				{
					idx = inter.size();
					inter.push_back(interV);
					path[idx] = {};
					mark[idx] = {};
					ori[idx] = {};
				}
				path[idx].push_back(eg.head);
				ori[idx].push_back(true);
				path[idx].push_back(es.tail);
				ori[idx].push_back(false);
				path[idx].push_back(es.head);
				ori[idx].push_back(true);
				path[idx].push_back(es.tail);
				ori[idx].push_back(false);
				mark[idx].push_back(false);
				mark[idx].push_back(false);
				mark[idx].push_back(false);
				mark[idx].push_back(false);
			}
		}
	}


	struct ChainState
	{
		struct Segment { Vertex inter; Polygon chain; };
		Vertex vrt;
		Vertex inter;
		Chainshard::CROSS chi;
		uint32_t pA;
		uint32_t pB;
		std::vector<Edge> eA;
		std::vector<Edge> eB;
		std::vector<Segment> seg;
	};

	Optional<ChainState> noneCase(ChainState state)
	{
		ChainState ret = state;
		// Make a lambda here
		auto poly = state.eA;
		auto chi = state.chi;
		bool isHead = state.chi == Chainshard::CROSS::HEAD;
		for(int i = 0; i < poly.size(); i++)
		{
			uint32_t place = (state.pA+i) % poly.size();
			Vertex diff = isHead ? poly[i].head : poly[i].tail;
			Vertex comp = isHead ? poly[i].tail : poly[i].head;
			if(eq(state.vrt,diff)) { return {true, ret}; }
		}
		return {false, ret};
	}

	ChainState initChainState(Polygon a, Polygon b)
	{
		ChainState ret;
		ret.eA = edgeThunk(a);
		ret.eB = edgeThunk(b);
		return ret;
	}

	ChainState cleanChainState(ChainState prev, Chainshard::Chainret um)
	{
		Vertex vrt = um.path;
		Vertex inter = um.inter;
		Chainshard::CROSS chirality = um.type;
		std::vector<ChainState::Segment> segment = 
			{ChainState::Segment{inter,Polygon{inter}}};
		return {vrt,inter,chirality,0,0,prev.eA,prev.eB,segment};
	}

	std::vector<Polygon> chain(Polygon a, Polygon b, Chainshard* shard)
	{
		// Initialize the state for the chain algorithm
		ChainState S = initChainState(a,b);
		std::vector<Polygon> ret;
		Optional<Chainshard::Chainret> um = shard->nextUnmarked();
		while(um.is)
		{
			S = cleanChainState(S, um.dat);
			// Path through the polygon
			while(S.seg.size() > 0)
			{
				// Start at the source, move and store loops
				auto next = shard->chainMark(S.vrt,S.chi);
				switch(next.type)
				{
					case Chainshard::CROSS::SINK:
					// When a sink is found weave a segment
					printf("SINK FOUND!\n");
					return {};
					break;
					case Chainshard::CROSS::NONE:
					{
						printf("NONE FOUND!\n");
						auto noneChange = noneCase(S);
						S = noneChange.dat;
					}
					return {};
					break;
					case Chainshard::CROSS::HEAD:
					printf("HEAD FOUND!\n");
					return {};
					break;
					case Chainshard::CROSS::TAIL:
					printf("TAIL FOUND!\n");
					return {};
					break;
				}
			}
			break;
		}
		return {};
	}	
};

std::vector<Polygon> geom::tempere(Polygon glass, Polygon frac)
{
	printf("In poly x poly tempere\n");
	chain::Chainshard* shard = new chain::Chainshard(glass, frac);
	return chain::chain(glass, frac, shard);
}
