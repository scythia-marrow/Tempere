// C++ imports
#include <vector>
#include <string>

// Module imports
#include "render.h"
#include "constraints.h"

#ifndef palette_h
#define palette_h
enum CONS_PALETTE : uint32_t
{
	PALETTE = CONS::GLUE
};

enum class palette : uint32_t
{
	RAND = (1 << 0),
	BLUES = (1 << 1),
	GREENS = (1 << 2),
	REDS = (1 << 3),
	TRANS = (1 << 4),
};

typedef struct color
{
	std::string name;
	double red;
	double green;
	double blue; 
} Color;

typedef struct pallete_t
{
	std::vector<Color> primary;
	std::vector<Color> complimentary;
	std::vector<Color> triadic;
} Palette;


Color color_hex(std::string hex);

Constraint palette(uint32_t number, double shade);

Palette pick_palette(Workspace*, uint32_t);
Color pick_color(Workspace*, Palette*, std::vector<Constraint>);
#endif
