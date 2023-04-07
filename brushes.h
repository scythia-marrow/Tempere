#include "render.h"
#include "palette.h"

#ifndef brushes_h
#define brushes_h
Callback solid(Workspace*, Segment, Brush);
Callback shape(Workspace*, Segment, Brush);
Callback line(Workspace*, Segment, Brush);
Callback shade(Workspace*, Segment, Brush);
Callback specularhighlight(Workspace*, Segment, Brush);

static Brush shape_brush
{
	.name = "shape",
	.priority = 0.7,
	.cons = 
	{
		"palette",
		"complexity",
		"size",
		"orientation"
	},
	.draw = shape
};

static Brush solid_brush
{
	.name = "solid",
	.priority = 1.0,
	.cons =
	{
		"palette",
		"complexity"
	},
	.draw = solid
};

static Brush line_brush
{
	.name = "line",
	.priority = 0.1,
	.cons =
	{
		"palette",
		"complexity",
		"size",
		"orientation"
	},
	.draw = line
};

static Brush specularhighlight_brush
{
	.name = "specularhighlight",
	.priority = 0.7,
	.cons =
	{
		"palette",
		"complexity",
		"size",
		"lighting"
	},
	.draw = specularhighlight
};
#endif
