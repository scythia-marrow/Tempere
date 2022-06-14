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
// Class definitions
class Layer;
class Workspace;
// Constraints are discrete or continuous, which are represented by a
// boolean mask or float, respectively.
typedef struct constraint
{
	const std::string name;
	uint32_t mask;
	double dial;
} Constraint;

/*
inline bool operator ==(const Constraint &a, const Constraint &b)
{
	return a.name == b.name;
}

inline bool operator <(const Constraint &a, const Constraint &b)
{
	return a.name < b.name;
}

inline bool operator >(const Constraint &a, const Constraint &b)
{
	return a.name > b.name;
}

namespace std
{
	template <> struct hash<Constraint>
	{
		std::size_t operator()(Operator const &n) const noexcept
		{
			return std::hash<std::string>{}(n.name);
		}
	};
}
*/

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
	const std::string name;
	const std::map<std::string, uint32_t> cons;
	std::function<Callback(Workspace*,struct op)> layout;
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
	const std::string name;
	// The prescidence of the brush
	double priority;
	// The constraints it uses
	const std::map<std::string, uint32_t> cons;
	// The actual drawing function
	std::function<Callback(Workspace*, struct segment, struct brush)> draw;
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


// A segment holds constraints and has a pointer to a canvas where it can paint
// As it must be copiable by value, it uses lambdas to create snapshots.
typedef struct segment
{
	// ID of this segment
	const uint32_t sid;
	// The canvas may be shared between segments
	cairo_surface_t* canvas;
	// Scale of coordinates -> pixels
	const double scale;
	// Layer segment is on.
	const uint32_t layer;
	// The boundary of the segment
	const std::vector<Vertex> boundary;
	// The constraints imposed on the segment from all sources
	const std::vector<Constraint> constraint;
	// Constructors for empty and null stuff
	segment() :
		sid{0},
		canvas{NULL},
		scale{0.0},
		layer{0},
		boundary{{}},
		constraint{{}} {};
	segment(const segment& s) : 
		sid{s.sid},
		canvas{s.canvas},
		scale{s.scale},
		layer{s.layer},
		boundary{s.boundary},
		constraint{s.constraint} {};
} Segment;

inline bool operator ==(const Segment &a, const Segment &b)
{
	return a.sid == b.sid;
}

inline bool operator <(const Segment &a, const Segment &b)
{
	return a.sid < b.sid;
}

inline bool operator >(const Segment &a, const Segment &b)
{
	return a.sid > b.sid;
}

namespace std
{
	template <> struct hash<Segment>
	{
		std::size_t operator()(Segment const &n) const noexcept
		{
			return std::hash<uint32_t>{}(n.sid);
		}
	};
}

// A layer holds a set of vertexes, ordered into segments and with
// logical relationships that require a class construction
// it also holds global and local constraints
class Layer
{
	struct shard
	{
		uint32_t sid;
		std::vector<uint32_t> vid;
	};
	std::vector<shard> perim;
	std::vector<Vertex> vertex;
	// Map: global -> local. Rev: local -> global.
	std::map<uint32_t,uint32_t> segMap;
	std::map<uint32_t,uint32_t> segRev;
	// Geometry relationships (purely local)
	std::map<uint32_t,std::vector<uint32_t>> geomRel;
	std::map<uint32_t,std::vector<Constraint>> constraint;
	// Initialization
	Layer(std::vector<Vertex>);
	// A single cache response
	Segment cache(uint32_t globalID, uint32_t localID);
	public:
		bool tempere(std::vector<Vertex()> boundary);
		std::vector<Segment> recache(Workspace* ws);
		std::vector<uint32_t> geom(Segment);
};

// A workspace holds layers and cairo drawing context.
class Workspace
{
	// There is always a background layer
	Layer* background;
	// A list of layers sorted by height. Min is zero.
	std::vector<uint32_t> height;
	std::map<uint32_t,Layer*> layer;
	// And a list of segments
	uint32_t registerSegmentID = 0;
	std::vector<Segment> segment;
	// A map of logical relationships between segments
	std::map<uint32_t,std::vector<uint32_t>> logic;
	// The constraints which direct brushes
	std::vector<Constraint> constraint;
	// The operators which can modify segments and impose constraints
	std::vector<Operator> op;
	// The brushes that consume constraints to produce art!
	std::vector<Brush> brush;
	// Initializer for the workspace
	Workspace();
	Workspace(cairo_surface_t*,std::vector<Vertex>);
	// A single layout step
	bool layoutStep();
	// The next layer on which boundary fits without envelopment
	uint32_t bounceLayer(uint32_t, std::vector<Vertex> boundary);
	// Function Utilities
	public:
		// Segment creation and manipulation
		uint32_t nextSegment();
		void ensureSegment(Segment);
		void addSegment(uint32_t, std::vector<Vertex> boundary);
		void setConstraint(Segment, std::vector<Constraint>);
		// Find segments with a certain match, default all segments
		std::vector<Segment> cut();
		std::vector<Segment> geomRel(Segment);
		std::vector<Segment> logRel(Segment);
		// Store caches
		std::map<Operator,std::map<Segment,uint32_t>> op_cache;
		std::map<Brush,std::map<Segment,uint32_t>> br_cache;
		// Main runtime functions
		bool addBrush(Brush);
		bool addOperator(Operator);
		bool addConstraint(Constraint);
		bool runTempere(uint32_t steps);
		bool render();
		// Brush specific public data
		const double scale;
		cairo_surface_t* canvas;
		std::function<double()> rand;
};
#endif
