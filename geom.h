// C imports
#include <stdlib.h>
#include <math.h>

// C++ impoorts
#include <cstdint>
#include <vector>
#include <tuple>

// Module imports
#include "optional.h"

#ifndef geom_h
#define geom_h
using opt::Optional;

namespace geom
{
	extern double EPS;
	double getEps();
	void setEps(double);

	typedef struct vector
	{
		double x;
		double y;
	} Vector;

	using Vertex = Vector;

	typedef struct pair
	{
		Vertex head;
		Vertex tail;
	} Edge;

	typedef std::vector<Vertex> Polygon;

	// Vector ops
	Vector add(Vector, Vector);
	Vector sub(Vector, Vector);
	Vector scale(Vector, double);
	Vector normalize(Vector);
	Vector vec(Vertex, Vertex);
	double angle(Vector, Vector);
	double cross(Vector, Vector);
	double dot(Vector, Vector);
	double magnitude(Vector);
	// Basic ops
	bool eq(double,double);
	bool eq(double,double,double);
	bool eq(Vertex, Vertex);
	bool eq(Vertex, Vertex, double);
	bool eq(Edge, Edge);
	bool eq(Polygon, Polygon);
	// Edge ops
	double arclen(Edge);
	double slope(Edge);
	bool direq(Edge, Edge);
	// Polygon thunks
	std::vector<Edge> edgeThunk(Polygon);
	Polygon polygonThunk(std::vector<Edge>);
	// Polygon Measures
	double perimeter(Polygon);
	double area(Polygon);
	double signed_area(Polygon);
	// Polygon Midpoints
	Vertex midpoint(Polygon);
	Vertex centroid(Polygon);
	// K-nn
	Vertex nearest_point(std::vector<Vertex>, Vertex);
	Vertex furthest_point(std::vector<Vertex>, Vertex);
	// Intersections
	Optional<Vertex> intersect_ray_line(Vertex, Vertex, Vertex, Vector);
	Optional<Vertex> intersect_ray_line(Edge e, Vertex origin, Vector dir);
	Optional<Vertex> intersect_edge_edge(Edge e1, Edge e2);
	std::vector<Vertex> intersect_ray_poly(Polygon, Vertex, Vector);
	std::vector<Polygon> tempere(Polygon,Polygon);
	// Polygon Tests
	bool on_edge(Edge e, Vertex v);
	bool on_edge(Polygon, Vertex);
	bool interior(Polygon, Vertex);
	bool interior(Polygon, Polygon);
	bool interior(Polygon, Polygon, bool);
	// Inclosure
	double dirangle(Edge, Vertex);
	double dirangle(Vertex, Vertex, Vertex);
	int32_t winding_number(Polygon, Vertex);
	// Projection
	Vector proj(Vector,Vector);
	// Search
	Optional<uint32_t> find(Polygon, Vertex);
	Optional<uint32_t> find(std::vector<Edge>, Edge);
	// Sort
	struct vrtcomp {
	bool operator() (const Vertex &a,const Vertex &b) const
	{
		if(eq(a,b)) { return false; }
		return std::tie(a.x,a.y) < std::tie(b.x,b.y);
	}
	};
	// A bidirectional weak ordering of edges! It's difficult math!
	struct edgecomp {
	bool operator() (const Edge &a, const Edge &b) const
	{
		if(eq(a,b)) { return false; }
		if(!eq(arclen(a),arclen(b)))
		{
			return arclen(a) < arclen(b);
		}
		if(!eq(abs(slope(a)),abs(slope(b))))
		{
			return abs(slope(a)) < abs(slope(b));
		}
		// Last comparison is location of midpoint
		Vertex midA = scale(add(a.head,a.tail),0.5);
		Vertex midB = scale(add(b.head,b.tail),0.5);
		return std::tie(midA.x,midA.y) < std::tie(midB.x,midB.y);
	}
	};
};
#endif
