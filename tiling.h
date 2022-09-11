#include "geom.h"
using namespace geom;

#ifndef tiling_h
#define tiling_h
typedef struct tile
{
	int id;
	int N;
	Vertex* vertex;
	int* neighbor;
} Tile;

typedef struct plane
{
	int N;
	Tile* tile;
} Plane;

// Different functions for tiling the plane!
Plane* tile_square(int, int);
Plane* tile_hexagon(double);
Plane* tile_triangle(double);
Plane* tile_penrose();

void print_plane(Plane*);
#endif
