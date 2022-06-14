#include <stdlib.h>

// C++ impoorts
#include <cstdint>
#include <vector>

#ifndef geom_h
#define geom_h
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
// Edge ops
double arclen(Edge);
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
Vertex intersect_ray_line(Vertex origin, Vector dir, Vertex v1, Vertex v2);
std::vector<Vertex> intersect_ray_poly(Vertex, Vector, Polygon);
Vertex intersect_edge_edge(Edge,Edge);
std::vector<Polygon> tempere(Polygon,Polygon);
// Polygon Tests
bool interior(Vertex, Polygon);
// Inclosure
uint32_t winding_number(Vertex, Polygon);
#endif
