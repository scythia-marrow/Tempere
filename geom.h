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
	bool eq(Vertex, Vertex);
	bool eq(Vertex, Vertex, double);
	bool eq(Edge, Edge);
	// Edge ops
	double arclen(Edge);
	double slope(Edge);
	// Polygon thunks
	std::vector<Edge> edgeThunk(Polygon);
	Polygon polygonThunk(std::vector<Edge>);
	// Polygon Measures
	double perimeter(Polygon);
	double signed_area(Polygon);
	// Polygon Midpoints
	Vertex midpoint(Polygon);
	Vertex centroid(Polygon);
	// K-nn
	Vertex nearest_point(Vertex, std::vector<Vertex>);
	Vertex furthest_point(Vertex, std::vector<Vertex>);
	// Intersections
	Optional<Vertex> intersect_ray_line(Vertex, Vector, Vertex, Vertex);
	Optional<Vertex> intersect_ray_line(Vertex origin, Vector dir, Edge e);
	Optional<Vertex> intersect_edge_edge(Edge e1, Edge e2);
	std::vector<Vertex> intersect_ray_poly(Vertex, Vector, Polygon);
	std::vector<Polygon> tempere(Polygon,Polygon);
	// Polygon Tests
	bool interior(Vertex, Polygon);
	bool interior(Polygon, Polygon);
	// Inclosure
	uint32_t winding_number(Vertex, Polygon);
	// Search
	Optional<uint32_t> find(Polygon, Vertex);
};
#endif
