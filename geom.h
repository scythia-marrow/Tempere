#include <stdlib.h>

// C++ impoorts
#include <cstdint>
#include <vector>

#ifndef geom_h
#define geom_h
typedef struct vertex
{
	double x;
	double y;
} Vertex;

// Vector ops
Vertex add(Vertex, Vertex);
Vertex scale(Vertex, double);
Vertex normalize(Vertex);
Vertex vec(Vertex, Vertex);
// Basic ops
bool eq(Vertex, Vertex);
double angle(Vertex, Vertex);
double cross(Vertex, Vertex);
double dot(Vertex, Vertex);
double l2_norm(Vertex, Vertex);
// Polygon Measures
double perimeter(std::vector<Vertex>);
double signed_area(std::vector<Vertex>);
// Polygon Midpoints
Vertex midpoint(std::vector<Vertex>);
Vertex centroid(std::vector<Vertex>);
// K-nn
Vertex nearest_point(Vertex, std::vector<Vertex>);
Vertex furthest_point(Vertex, std::vector<Vertex>);
// Intersections
Vertex intersect_ray_line(Vertex origin, Vertex dir, Vertex v1, Vertex v2);
std::vector<Vertex> intersect_ray_poly(Vertex, Vertex, std::vector<Vertex>);
// Inclosure
uint32_t winding_number(Vertex, std::vector<Vertex>);
#endif
