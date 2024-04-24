#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <cairo.h>

// Geometry definition(s)
#include "geom.h"
using namespace geom;

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
	uint32_t type;
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
	std::string name;
	// The constraints it uses for different types
	std::set<std::string> cons;
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
	std::string name;
	// The prescidence of the brush
	double priority;
	// The constraints it uses
	std::set<std::string> cons;
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
// TODO: move relationships and caches into here using lambdas?
typedef struct segment
{
	// ID of this segment
	uint32_t sid;
	// The canvas may be shared between segments
	cairo_surface_t* canvas;
	// Scale of coordinates -> pixels
	double scale;
	// Layer segment is on.
	uint32_t layer;
	// The boundary of the segment
	std::vector<Vertex> boundary;
	// The constraints imposed on the segment from all sources
	std::vector<Constraint> constraint;
	// Constructors for empty and null stuff
	segment& operator=(const segment&) & = default;
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
// TODO: make layers composible with workspaces. Right now they store global ids
// without tracking which workspace is giving them said ids.
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
	std::map<uint32_t,std::set<uint32_t>> geomRel;
	std::map<uint32_t,std::vector<Constraint>> constraint;
	// Add a segment (purely local)
	uint32_t addsegment(std::vector<segment>&,Polygon);
	uint32_t ensureVid(Vertex);
	// Logical relationships (semi-local, sid is local lid is global)
	std::map<uint32_t,std::set<uint32_t>> logicRel;
	// A single cache response
	Segment cache(Workspace*, uint32_t gid, uint32_t sid, uint32_t height);
	// All public functions return workspace-specific ids. (global)
	public:
		// Initialization
		Layer(std::vector<Vertex>);
		void tempere(std::vector<Vertex> boundary);
		// Data access
		std::set<uint32_t> geom(Segment);
		std::set<uint32_t> logic(uint32_t);
		// Segments perform blocking and unification between workspaces
		void updateConstraint(Segment, std::vector<Constraint>);
		// Link a segment to a link id
		void linkLogical(Segment, uint32_t lid);
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
	/* Persistant data, remains intact after layout or draw */
	// The constraints which direct brushes
	std::vector<Constraint> constraint;
	// The operators which can modify segments and impose constraints
	std::vector<Operator> oper;
	// The brushes that consume constraints to produce art!
	std::vector<Brush> brush;
	// There is always a background layer
	Layer* background;
	// A list of layers sorted by height. Min is zero.
	std::vector<uint32_t> height;
	std::map<uint32_t,Layer*> layer;
	// A set of all logical relationships in the workspace
	std::vector<uint32_t> logic;
	// Static caches for dependency injection TODO: actually implement
	std::map<Operator,std::map<uint32_t,uint32_t>> op_internal;
	std::map<Brush,std::map<uint32_t,uint32_t>> br_internal;
	/* Volatile data, regenerated after layout or draw */
	uint32_t registerSegmentID = 0;
	std::vector<Segment> segment;
	std::function<uint32_t()> sidGen()
	{
		uint32_t monoid = segment.size();
		return [=]() mutable -> uint32_t { return monoid++; };
	};
	std::map<uint32_t,std::set<uint32_t>> linkMap;
	/* Private functions */
	// The next layer on which boundary fits without envelopment
	Layer* addLayer(uint32_t height, std::vector<Vertex> boundary);
	uint32_t bounceLayer(uint32_t, std::vector<Vertex> boundary);
	// A single layout step
	bool ensureReadyLayout();
	double layoutStep(std::vector<double> zipfs);
	// A single draw step
	std::vector<Callback> drawSegment(Segment s, std::vector<double> zipf);
	// Function Utilities
	bool ensureReadyRender();
	// Public operations, called by the runtime directly or through DI
	public:
		// Initializer for the workspace
		// Workspace();
		Workspace(cairo_surface_t*,std::vector<Vertex>,double);
		Workspace(const Workspace&,cairo_surface_t*);
		// Find segments with a certain match, default all segments
		std::vector<Segment> cut();
		std::set<Segment> geomRel(Segment);
		std::set<Segment> logicRel(Segment);
		// Store caches used by operators, volatile TODO: fix volatile
		std::map<Operator,std::map<Segment,uint32_t>> op_cache;
		std::map<Brush,std::map<Segment,uint32_t>> br_cache;
		// Main runtime functions
		bool addBrush(Brush);
		bool addOperator(Operator);
		bool addConstraint(Constraint);
		bool runTempere(uint32_t steps,bool);
		bool render();
		bool renderDebug();
		// Operator specific public functions for segment manipulations
		void setConstraint(Operator, Segment, std::vector<Constraint>);
		// TODO: may need to add operator verification here
		void addSegment(
			Operator op,
			uint32_t layer,
			std::vector<Vertex> boundary,
			uint32_t mark);
		void linkSegment(Operator, Segment, Segment);
		// Brush specific public data TODO: add brush verification?
		double scale() { return const_scale; }
		cairo_surface_t* canvas() { return const_canvas; };
		std::function<double()> rand;
};
#endif
