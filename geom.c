// C imports
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cassert>

// C++ imports
#include <map>
#include <functional>

#include "geom.h"
#include "optional.h"
#include "tempere.h"
using opt::Optional;

double geom::EPS = 0.001;

void geom::setEps(double eps) { EPS = eps; }
double geom::getEps() { return EPS; }

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
	return eq(a,b,EPS);
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

bool geom::direq(Edge e1, Edge e2)
{
	if(eq(e1.head,e2.head) && eq(e1.tail, e2.tail)) { return true; }
	return false;
}

// Have to compare all edges TODO: EFFICIENCY. Implicit comparison?
bool geom::eq(Polygon p1, Polygon p2)
{
	if(p1.size() != p2.size()) { return false; }
	std::vector<Edge> edge = edgeThunk(p2);
	for(auto e : edgeThunk(p1)) { if(!find(edge,e).is) { return false; } }
	return true;
}

bool geom::eq(double s1, double s2)
{
	return eq(s1,s2,EPS);
}

bool geom::eq(double s1, double s2, double eps)
{
	bool scalarequals = s1 > s2 ? ((s1 - s2) < eps) : ((s2 - s1) < eps);
	return scalarequals;
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
	if(eq(mag_sum,0.0)) { return 0.0; }
	double dot_sum = dot(a,b);
	double quot = dot_sum / mag_sum;
	return acos(quot);
}

double geom::dirangle(Vertex a, Vertex b, Vertex c)
{
	// Degenerate case
	if(eq(a,c)) { return 0.0; }
	// The first vector is from b -> a, second is from b -> c
	Vector head = vec(b,a);
	Vector tail = vec(b,c);
	// Get the Z cross product component to ensure this is righthanded
	double z = (head.x * tail.y) - (head.y * tail.x);
	if(eq(z,0.0)) { return angle(head,tail); }
	return z > 0.0 ? angle(head,tail) : angle(head,tail) + M_PI;
}

double geom::dirangle(Edge e, Vertex c)
{
	return dirangle(e.head,e.tail,c);
}

Vector geom::proj(Vector base, Vector vec)
{
	return scale(base, dot(base,vec) / magnitude(vec));
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
	for(uint32_t h = 0; h < boundary.size(); h++)
	{
		uint32_t t = (h + boundary.size() + 1) % boundary.size();
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

double geom::area(Polygon poly) { return std::abs(signed_area(poly)); }

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
	if(poly.size() < 1) { return centroid; }
	double area_s = signed_area(poly);
	for(auto e : edgeThunk(poly))
	{
		double a = cross(e.head, e.tail);
		centroid.x += (e.head.x + e.tail.x) * a;
		centroid.y += (e.head.y + e.tail.y) * a;
	}
	// If the area is zero we have a zero-size polygon, so the centroid
	// is just the polygon midpoint TODO: check the math on this case
	if(eq(area_s,0.0)) { return midpoint(poly); }

	centroid.x /= (6.0 * area_s);
	centroid.y /= (6.0 * area_s);

	//printf("Centroid: (%f,%f)\n", centroid.x, centroid.y);
	return centroid;
}

Optional<Vertex> geom::intersect_ray_line(Edge e, Vertex origin, Vector dir)
{
	Vector p1 = sub(origin, e.head);
	Vector p2 = sub(e.tail, e.head);
	Vector p3 {-1.0f * dir.y, dir.x};

	double D = dot(p2,p3);
	// Zero implies the ray is codirectional to the line
	if(eq(D,0.0))
	{
		// Codirectional to p1 as well means the origin is on the line
		double Dt = dot(p1,p3);
		if(eq(Dt,0.0)) { return { true, origin }; }
		return { false, {0.0,0.0} };
	}
	
	double t1 = cross(p2, p1) / D;
	double t2 = dot(p1,p3) / D;

	Vertex interpoint = add(origin, (Vertex)scale(dir,t1));
	if(eq(t1,0.0))
	{
		if(eq(t2,0.0) || eq(t2,1.0)) { return {true, interpoint }; }
		if(t2 < 0.0 || t2 > 1.0) { return { false, interpoint }; }
		return { true, interpoint };
	}
	if(t1 < 0.0) { return { false, interpoint }; }
	if(eq(t2,0.0) || eq(t2,1.0)) { return { true, interpoint }; }
	if(t2 < 0.0 || t2 > 1.0) { return { false, interpoint }; }
	return { true, interpoint };
}

Optional<Vertex> geom::intersect_ray_line(
	Vertex v1, Vertex v2,
	Vertex o, Vector dir)
{
	return intersect_ray_line(Edge{v1,v2}, o, dir);
}

std::vector<Vertex> geom::intersect_ray_poly(Polygon poly, Vertex o, Vector dir)
{
	std::vector<Vertex> ret;
	for(auto e : edgeThunk(poly))
	{
		Optional<Vertex> vrt = intersect_ray_line(e, o, dir);
		// TODO: why is this here?
		if(!vrt.is) { continue; }
		ret.push_back(vrt.dat);
	}
	return ret;
}

Optional<Vertex> geom::intersect_edge_edge(Edge head, Edge tail)
{
	Vector ray = sub(head.tail, head.head);
	Optional<Vertex> interO = intersect_ray_line(tail, head.head, ray);
	// Check if the intersection is within the edge
	if(!interO.is) { return interO; }
	Vector interRay = vec(head.head,interO.dat);
	if(magnitude(interRay) > magnitude(ray)) { interO.is = false; }
	if(eq(magnitude(interRay), magnitude(ray))) { interO.is = true; }
	return interO;
}

Vertex geom::nearest_point(Polygon vtx, Vertex o)
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

Vertex geom::furthest_point(Polygon vtx, Vertex o)
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

// TODO: what about edge intersections?
bool geom::interior(Polygon poly, Vertex v)
{
	bool ret = winding_number(poly, v) != 0;
	return ret;
}

bool geom::interior(Polygon out, Polygon in, bool closed)
{
	// Points on the edge are ignored
	std::vector<Vertex> ignore;
	for(auto vrt : in) { if(on_edge(out,vrt)) { ignore.push_back(vrt); } }
	// Special cases for closed and open polygons
	if(ignore.size() == out.size()) { return closed; }
	if(!closed && ignore.size() > 0) { return false; }
	// Finding a single exterior, non-edge point is enough
	uint32_t I = 0;
	for(auto vrt : in)
	{
		if(I < ignore.size() && eq(vrt,ignore[I])) { I++; continue; }
		if(!interior(out,vrt)) { return false; }
	}
	return true;
}

bool geom::interior(Polygon out, Polygon in) { return interior(out,in,true); }


bool geom::on_edge(Edge e, Vertex v)
{
	double angle = dirangle(e, v);
	if(!eq(angle,0.0)) { return false; }
	if(eq(e.head,v) || eq(e.tail,v)) { return true; }
	double mH = magnitude(vec(e.head,v));
	double mT = magnitude(vec(e.tail,v));
	if(eq(mH + mT,arclen(e))) { return true; }
	return false;
}

bool geom::on_edge(Polygon p, Vertex v)
{
	for(auto e : edgeThunk(p)) { if(on_edge(e,v)) { return true; }}
	return false;
}

int32_t geom::winding_number(Polygon poly, Vertex v)
{
	auto dir_left = [](Vertex h0, Vertex t0, Vertex p0)
	{
		double left = (
			((h0.x - p0.x) * (t0.y - p0.y))
			- ((t0.x - p0.x) * (h0.y - p0.y))
		);
		return left;
	};
	int32_t wn = 0;
	for(auto e : edgeThunk(poly))
	{
		Vertex H = e.head;
		Vertex T = e.tail;
		bool crossup = false;
		bool crossdown = false;
		int32_t scale = 2;
		if(eq(H.x,v.x)) { scale--; }
		if(eq(T.x,v.x)) { scale--; }
		if(scale == 0) { continue; }
		if(scale == 1)
		{
			if(eq(T.x,v.x) && (H.x < v.x)) { crossup = true; }
			if(eq(T.x,v.x) && (H.x > v.x)) { crossdown = true; }
			if(eq(H.x,v.x) && (T.x < v.x)) { crossdown = true; }
			if(eq(H.x,v.x) && (T.x > v.x)) { crossup = true; }
		}
		else
		{
			crossup = (H.x < v.x) && (T.x > v.x);
			crossdown = (H.x > v.x) && (T.x < v.x);
		}
		assert(!(crossup && crossdown));
		if(!(crossup || crossdown)) { continue; }
		double left = dir_left(H,T,v);
		if(eq(left,0.0)) { continue; }
		bool onleft = left > 0.0;
		if(crossup && onleft) { wn += scale; }
		if(crossdown && !onleft) { wn -= scale; }
	}
	return (wn/2);
}

Optional<uint32_t> geom::find(Polygon poly, Vertex vert)
{
	for(uint32_t i = 0; i < poly.size(); i++)
	{
		if(eq(poly[i], vert)) { return {true,i}; }
	}
	return {false,0};
}

Optional<uint32_t> geom::find(std::vector<Edge> poly, Edge edge)
{
	for(uint32_t i = 0; i < poly.size(); i++)
	{
		if(eq(poly[i], edge)) { return {true,i}; }
	}
	return {false,0};
}

std::vector<Polygon> geom::tempere(Polygon glass, Polygon frac)
{
	if(frac.size() < 2) { return { glass }; }
	chain::Chainshard* shard = new chain::Chainshard(glass, frac);
	return chain::chain(shard);
}
