#include <stdlib.h>

// C++ impoorts
#include <cstdint>
#include <vector>

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
		Vector v = vec(a,b);
		if(magnitude(v) < EPS) { return false; }
		return magnitude(a) < magnitude(b); 
	}
	};	
};
#endif
