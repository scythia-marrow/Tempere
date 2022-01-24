#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "geom.h"
#include "tiling.h"

//TODO: The strategy! Make a tiling of the plane, determine boundary and bulk
//	rates of crystal formation, then simulate from a set of seeds!
Plane* tile_square(int dimX, int dimY)
{
	// Each tile has 4 neighbors
	int N = 4;
	// Tile centers depend on location
	int numTiles = dimX * dimY;
	double xDelta = 1.0 / dimX;
	double yDelta = 1.0 / dimY;

	// Allocate a set of tiles
	Plane* plane = (Plane*)malloc(sizeof(Plane));
	plane->N = numTiles;
	plane->tile = (Tile*)malloc(sizeof(Tile)*numTiles);

	// Tile the plane with evenly placed squares
	int i = 0;
	for(int x = 0; x < dimX; x++)
	{
		for(int y = 0; y < dimY; y++) {
			// Get each corner point
			double xMin = x*xDelta;
			double xMax = (x+1)*xDelta;
			double yMin = y*yDelta;
			double yMax = (y+1)*yDelta;
			// Make the tiles
			Tile newTile;
			newTile.id = i;
			newTile.N = N;
			newTile.vertex = (Vertex*)malloc(sizeof(Vertex)*N);
			newTile.vertex[0] = {xMin,yMin};
			newTile.vertex[1] = {xMax,yMin};
			newTile.vertex[2] = {xMax,yMax};
			newTile.vertex[3] = {xMin,yMax};
			// Link the neighbors
			int xLeft = (dimX + x - 1)%dimX;
			int xRight = (dimX + x + 1)%dimX;
			int yLeft = (dimY + y - 1)%dimY;
			int yRight = (dimY + y + 1)%dimY;
			newTile.neighbor = (int*)malloc(sizeof(int)*4);
			newTile.neighbor[0] = x+(yLeft)*dimX;
			newTile.neighbor[1] = xRight+y*dimX;
			newTile.neighbor[2] = x+(yRight)*dimX;
			newTile.neighbor[3] = xLeft+y*dimX;
			plane->tile[i] = newTile;
			i++;
		}
	}
	return plane;
}

Plane* tile_hexagon(double size)
{
	return NULL;
}

Plane* tile_triangle(double size)
{
	return NULL;
}

Plane* tile_penrose()
{
	return NULL;
}

void print_plane(Plane* plane)
{
	for(int i = 0; i < plane->N; i++)
	{
		Tile tile = plane->tile[i];
		printf("Tile %i vertexes:\n", i);
		for(int v = 0; v < tile.N; v++)
		{
			int nID = tile.neighbor[v];
			Tile neighbor = plane->tile[nID];
			Vertex head = tile.vertex[v];
			Vertex tail = tile.vertex[(tile.N + v + 1)%tile.N];
			printf("\t %i (%i): \t%f,%f -> %f,%f\n",
				neighbor.id, nID,
				head.x, head.y,
				tail.x, tail.y);
		}
		
	}
}

#ifdef TEST_TILING
int main()
{
	Plane* plane = tile_square(5,5);
	print_plane(plane);
	return 0;
}
#endif
