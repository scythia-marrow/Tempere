#include "render.h"
#include "palette.h"

#ifndef operators_h
#define operators_h

Callback symmetry(Workspace*, Operator);
Callback figureandground(Workspace*, Operator);
Callback gradient(Workspace*, Operator);
Callback focal(Workspace*, Operator);

static Operator symmetry_operator
{
	.name = "symmetry",
	.cons =
	{
		"complexity",
		"orientation"
	},
	.layout = symmetry
};

static Operator figure_and_ground_operator
{
	.name = "figure_and_ground",
	.cons =
	{
		"complexity",
		"size"
	},
	.layout = figureandground
};

static Operator focal_point_operator
{
	.name = "focal_point",
	.cons =
	{
		"complexity",
		"orientation"
	},
	.layout = focal
};

static Operator gradient_operator
{
	.name = "gradient",
	.cons = 
	{
		"complexity",
		"orientation",
		"size",
		"palette"
	},
	.layout = gradient
};
#endif
