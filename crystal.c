#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "tiling.h"

typedef struct node
{
	double density;
	Tile* tile;
} Node;

typedef struct crystal
{

} Crystal;


typedef struct boundary
{
	int num;
	
} Boundary;

void seed_plane()
{

}


#ifdef TEST_CRYSTAL
int main()
{
	Plane* plane = tile_square(10,10);
	printf("Hello World!\n");
	return 0;
}
#endif
