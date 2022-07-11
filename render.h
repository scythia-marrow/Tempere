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
// Basic types for easy id updates
//typedef uint32_t SID;
//typedef uint32_t LID;
//typedef uint32_t MARK;
// Class definitions
class Layer;
class Workspace;
// Constraints are discrete or continuous, which are represented by a
// boolean mask or float, respectively.
typedef struct constraint
{
	std::string name;
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
		std::size_t operator()(Constraint const &n) const noexcept
		{
			return std::hash<std::string>{}(n.name);
		}
	};
}*/


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
	// The constraints it uses, both name and mask for different types
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
	// The constraints it uses, both name and mask for different types
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
	segment(uint32_t sid,
		cairo_surface_t* canvas,
		double scale,
		uint32_t layer,
		std::vector<Vertex> bound,
		std::vector<Constraint> con) : 
		sid{sid},
		canvas{canvas},
		scale{scale},
		layer{layer},
		boundary(bound),
		constraint(con) {};
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
	struct segment
	{
		uint32_t sid;
		std::vector<uint32_t> vid;
	};
	std::vector<segment> shard;
	std::vector<Vertex> vertex;
	// Map: global -> local. Rev: local -> global.
	std::map<uint32_t,uint32_t> segMap;
	std::map<uint32_t,uint32_t> segRev;
	// Geometry relationships (purely local)
	std::map<uint32_t,std::vector<uint32_t>> geomRel;
	std::map<uint32_t,std::vector<Constraint>> constraint;
	// A single cache response
	Segment cache(Workspace*, uint32_t gid, uint32_t sid, uint32_t height);
	public:
		// Initialization
		Layer(std::vector<Vertex>);
		bool tempere(std::vector<Vertex> boundary);
		std::vector<uint32_t> geom(Segment);
		// Set constraint
		void updateConstraint(Segment, std::vector<Constraint>);
		// Get all uncached segments
		std::vector<Segment> unmappedSegment(
			Workspace*,
			uint32_t,
			std::function<uint32_t()>);
		// Force a full recache
		std::vector<Segment> recache(
			Workspace* ws,
			uint32_t height,
			std::function<uint32_t()> gidgen);
};

// A workspace holds layers and cairo drawing context.
class Workspace
{
	// Scale and canvas are private, read-only
	double const_scale;
	cairo_surface_t* const_canvas;
	// There is always a background layer
	Layer* background;
	// A list of layers sorted by height. Min is zero.
	std::vector<uint32_t> height;
	std::map<uint32_t,Layer*> layer;
	// And a list of segments
	uint32_t registerSegmentID = 0;
	std::vector<Segment> segment;
	std::function<uint32_t()> sidGen()
	{
		uint32_t monoid = segment.size();
		return [=]() mutable -> uint32_t { return monoid++; };
	};
	// A map of logical relationships between segments
	std::map<uint32_t,std::vector<uint32_t>> logic;
	// The constraints which direct brushes
	std::vector<Constraint> constraint;
	// The operators which can modify segments and impose constraints
	std::vector<Operator> oper;
	// The brushes that consume constraints to produce art!
	std::vector<Brush> brush;
	// A single layout step
	bool ensureReadyLayout();
	double layoutStep(std::vector<double> zipfs);
	// A single draw step
	std::vector<Callback> drawSegment(Segment s, std::vector<double> zipf);
	// The next layer on which boundary fits without envelopment
	Layer* addLayer(uint32_t height, std::vector<Vertex> boundary);
	uint32_t bounceLayer(uint32_t, std::vector<Vertex> boundary);
	// Function Utilities
	bool ensureReadyRender();
	std::map<Operator,std::map<uint32_t,uint32_t>> op_internal;
	std::map<Brush,std::map<uint32_t,uint32_t>> br_internal;
	public:
		// Initializer for the workspace
		Workspace();
		Workspace(cairo_surface_t*,std::vector<Vertex>);
		// Segment creation and manipulation
		void ensureSegment(Segment);
		void setConstraint(Segment, std::vector<Constraint>);
		void addSegment(
			Operator op,
			uint32_t layer,
			std::vector<Vertex> boundary,
			uint32_t mark);
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
		double scale() { return const_scale; }
		cairo_surface_t* canvas() { return const_canvas; };
		std::function<double()> rand;
};
#endif
