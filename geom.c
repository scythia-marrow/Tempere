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
			printf("WN %u\n",winding_number(vrt,out));
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

std::vector<Polygon> geom::tempere(Polygon glass, Polygon shard)
{
	printf("In poly x poly tempere\n");
	// TODO: check if the polygons are equivalent when no edges found.
	// Store all edge intersections, associate them with edges
	std::vector<Vertex> source;
	std::map<uint32_t,std::vector<Edge>> edge;
	// Find all edge intersections
	for(auto eg : edgeThunk(glass))
	{
		for(auto es : edgeThunk(shard))
		{
			if(eq(eg,es)) { continue; }
			Optional<Vertex> interO = intersect_edge_edge(eg, es);
			if(!interO.is) { continue; }
			Vertex inter = interO.dat;
			Optional<uint32_t> idxO = find(source,inter);
			uint32_t idx;
			if(idxO.is) {idx = idxO.dat; }
			else
			{
				idx = source.size();
				edge[idx] = {};
				source.push_back(inter);
			}
			edge[idx].push_back(eg);
			edge[idx].push_back(es);
		}
	}
	printf("Found %i edges\n",source.size());
	return {};
}
