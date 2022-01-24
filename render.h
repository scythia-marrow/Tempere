#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <cairo.h>

// Geometry definition(s)
#include "geom.h"

// (Limited C++ imports) GIB STRING CLASS GCC!
#include <string>
#include <vector>
#include <random>
#include <map>
#include <functional>
#include <set>

#ifndef render_h
#define render_h
// Constraints are discrete or continuous, which are represented by a
// boolean mask or float, respectively.
typedef struct constraint
{
	std::string name;
	uint32_t mask;
	double dial;
} Constraint;

// A valued callback!
typedef struct callback
{
	bool usable;
	double match;
	double priority;
	std::function<void()> callback;
} Callback;

// An operator segments the space and applies constraints.
// Basically, it plans the layout of the drawing.
typedef struct op
{
	std::string name;
	const std::map<std::string, uint32_t> cons;
	std::function<Callback(
		struct workspace*,
		struct op)> layout;
} Operator;

inline bool operator ==(const Operator &a, const Operator &b)
{
	return a.name == b.name;
}

inline bool operator <(const Operator &a, const Operator &b)
{
	return a.name < b.name;
}

inline bool operator >(const Operator &a, const Operator &b)
{
	return a.name > b.name;
}

namespace std
{
	template <> struct hash<Operator>
	{
		std::size_t operator()(Operator const &n) const noexcept
		{
			return std::hash<std::string>{}(n.name);
		}
	};
}

// A brush stores a name, the constraints it reads, and a drawing function
typedef struct brush
{
	// The name of this brush
	std::string name;
	// The prescidence of the brush
	double priority;
	// The constraints it uses
	const std::map<std::string, uint32_t> cons;
	// The actual drawing function
	std::function<Callback(
		struct workspace*,
		struct segment*,
		struct brush)> draw;
} Brush;

inline bool operator ==(const Brush &a, const Brush &b)
{
	return a.name == b.name;
}

inline bool operator <(const Brush &a, const Brush &b)
{
	return a.name < b.name;
}

inline bool operator >(const Brush &a, const Brush &b)
{
	return a.name > b.name;
}
namespace std
{
	template <> struct hash<Brush>
	{
		std::size_t operator()(Brush const &n) const noexcept
		{
			return std::hash<std::string>{}(n.name);
		}
	};
}


// A segment holds constraints and has a pointer to a canvas where it can
// paint
typedef struct segment
{
	// The layer the segment lies on, for ordering purposes
	int layer;
	double scale;
	// Constraints about types of brushes to use / parameters
	std::vector<Constraint> constraint;
	// Other segments directly related to this one.
	std::set<struct segment*> link;
	// The boundary of the segment
	std::vector<Vertex> boundary;
	// The canvas may be shared between segments
	cairo_surface_t* canvas;
} Segment;

// A workspace holds the root segment and cairo drawing context.
typedef struct workspace
{
	// Random generators for brushes to use
	std::function<double()> rand;
	// Cache for operators to use marking segments
	std::map<Operator,std::map<Segment*,uint32_t>> op_cache;
	// Cache for brushes to use marking segments
	std::map<Brush,std::map<Segment*,uint32_t>> br_cache;
	// The root segment that holds the entire artwork
	Segment root;
	std::vector<Segment*> segment;
	// A lambda for storing layer information
	uint32_t min_layer = 0;
	uint32_t max_layer = 0;
	std::function<uint32_t(Segment*,uint32_t)> layer_relative =
		[=](Segment* s, uint32_t diff) mutable -> uint32_t
		{
				uint32_t layer = s->layer + diff;
				min_layer =
					layer < min_layer ? layer : min_layer;
				max_layer =
					layer > max_layer ? layer : max_layer;
				return layer;
		};
	// The constraints which direct brushes
	std::vector<Constraint> constraint;
	// The operators which can modify segments and impose constraints
	std::vector<Operator> op;
	// The brushes that consume constraints to produce art!
	std::vector<Brush> brush;
} Workspace;
#endif
