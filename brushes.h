#include "render.h"
#include "palette.h"

#ifndef brushes_h
#define brushes_h
Callback solid(Workspace*, Segment, Brush);
Callback shape(Workspace*, Segment, Brush);
Callback line(Workspace*, Segment, Brush);
Callback shade(Workspace*, Segment, Brush);

static Brush shape_brush
{
	.name = "shape",
	.priority = 0.7,
	.cons = 
	{
		{"palette", CONS_PALETTE::PALETTE},
		{"complexity", CONS::COMPLEXITY},
		{"size", CONS::SIZE},
		{"orientation", CONS::ORIENTATION}
	},
	.draw = shape
};

static Brush solid_brush
{
	.name = "solid",
	.priority = 1.0,
	.cons =
	{
		{"palette", CONS_PALETTE::PALETTE},
		{"complexity", CONS::COMPLEXITY}
	},
	.draw = solid
};

static Brush line_brush
{
	.name = "line",
	.priority = 0.1,
	.cons =
	{
		{"palette", CONS_PALETTE::PALETTE},
		{"complexity", CONS::COMPLEXITY},
		{"size", CONS::SIZE},
		{"orientation", CONS::ORIENTATION}
	},
	.draw = line
};
#endif
