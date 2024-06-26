// C imports
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cairo-svg.h>
#include <cassert>

// C++ imports
#include <vector>
#include <deque>
#include <functional>
#include <algorithm>
#include <random>
#include <chrono>

// module imports
#include "geom.h"
#include "render.h"
#include "palette.h"
#include "brushes.h"
#include "operators.h"
#include "constraints.h"

// debug imports
#include <iostream>

// TODO: need a header file for these companion definitions
Callback weighted_choice(Workspace* ws, std::vector<Callback> cbs, double d);

// We weight operators and brushes by Zip's law to make distinct pictures with
// the same operators. Think different "languages" based on the same phonemes
std::vector<double> zipfs_weight(Workspace* ws, int N)
{
	// Make weights according to Zipf's distribution
	std::vector<double> weight;
	for(int i = 0; i < N; i++) { weight.push_back(1.0 / (N+1)); }
	// Shuffle them
	auto randint = [=](int i) -> int { return int(ws->rand() * i); };
	std::random_shuffle(weight.begin(), weight.end(), randint);
	return weight;
}

// Insert Callback b into vector cb in order of decreasing priority
void insertsort(std::vector<Callback> &cb, Callback b)
{
	int i; int s = cb.size();
	for(i = 0; i < s && cb[i].priority > b.priority; i++);
	cb.insert(cb.begin() + i, b);
}

// Layer class definitions
Layer::Layer(std::vector<Vertex> start)
{
	// Store the canvas boundary vertexes
	std::vector<uint32_t> vid;
	for(uint32_t v = 0; v < start.size(); v++)
	{
		vertex.push_back(start[v]);
		vid.push_back(v);
	}
	segment root = {0, vid};
	shard.push_back(root);
}

Segment Layer::cache(Workspace* ws, uint32_t gid, uint32_t id, uint32_t height)
{
	// Cache the global id and reverse
	segMap[id] = gid;
	segRev[gid] = id;
	// Create a new Segment from local knowledge
	segment sh = shard[id];
	std::vector<Vertex> bound;
	for(auto v : sh.vid) { bound.push_back(vertex[v]); }
	std::vector<Constraint> con = constraint[id];
	Segment ret = {gid, ws->canvas(), ws->scale(), height, bound, con};
	return ret;
}

std::vector<Segment> Layer::recache(
	Workspace* ws, uint32_t height, std::function<uint32_t()> gidgenerator)
{
	std::vector<Segment> ret;
	for(auto sh : shard)
	{
		uint32_t gid = gidgenerator();
		ret.push_back(cache(ws, gid, sh.sid, height));
	}
	return ret;
}

std::set<uint32_t> Layer::geom(Segment s) { return geomRel[segRev[s.sid]]; }
std::set<uint32_t> Layer::logic(uint32_t lid)
{
	if(!logicRel.count(lid)) { return {}; }
	return logicRel[lid];
}

void Layer::linkLogical(Segment s, uint32_t lid)
{
	uint32_t sid = segMap[s.sid];
	if(!logicRel.count(lid)) { logicRel[lid] = {}; }
	logicRel[lid].insert(sid);
}

uint32_t Layer::ensureVid(Vertex vrt)
{
	auto found = geom::find(vertex,vrt);
	if(found.is) { return found.dat; }
	vertex.push_back(vrt);
	return ensureVid(vrt);
}

uint32_t Layer::addsegment(std::vector<segment> &base, Polygon poly)
{
	uint32_t newsid = base.size();
	std::vector<uint32_t> id;
	for(auto vrt : poly) { id.push_back(ensureVid(vrt)); }
	base.push_back({newsid,id});
	return newsid;
}

void Layer::tempere(std::vector<Vertex> boundary)
{
	// Updated segments and constraints
	std::vector<segment> shatter;
	std::map<uint32_t,std::vector<Constraint>> shattercon;
	// Check for coverage and intersections
	for(auto p : shard)
	{
		// Shatter the shard if needed
		std::vector<Vertex> perimiter;
		for(auto v : p.vid) { perimiter.push_back(vertex[v]); }
		// Run tempere on the shard
		auto piece = geom::tempere(perimiter, boundary);
		// Store the pieces produced in new shards
		for(auto poly : piece)
		{
			uint32_t newid = addsegment(shatter, poly);
			shattercon[newid] = constraint[p.sid];
		}
	}
	// For now just straight replace all shards with the new stuff
	assert(shard.size() <= shatter.size());
	// printf("\rShattered %d into %d\n",shard.size(),shatter.size());
	// fflush(stdout);
	// DEBUG!
	if(shard.size() == shatter.size())
	{
		for(auto p : shard)
		{
			std::vector<Vertex> perimiter;
			for(auto v : p.vid) { perimiter.push_back(vertex[v]); }
			auto piece = geom::tempereDebug(perimiter, boundary);
		}
	}
	shard = shatter;
	constraint = shattercon;
	// Store a map of shards and their verticies for local relationships
	std::map<uint32_t,std::vector<uint32_t>> localMap;
	for(auto p : shard)
	{
		for(auto id : p.vid)
		{
			if(!localMap.count(id)) { localMap[id] = {}; }
			localMap[id].push_back(p.sid);
		}
	}
	// Clear the local relationships
	geomRel = {};
	// Create the local relationships anew
	for(auto p : shard)
	{
		for(auto id : p.vid)
		{
			for(auto sid : localMap[id])
			{
				if(!geomRel.count(p.sid))
				{
					geomRel[p.sid] = {};
				}
				geomRel[p.sid].insert(sid);
			}
		}
	}
}

std::vector<Segment> Layer::unmappedSegment(
	Workspace* ws, uint32_t height, std::function<uint32_t()> gidgen)
{
	// std::cout << "TOTAL SHARDS " << shard.size() << std::endl;
	// Make sure all segments are mapped, make a list of unmapped segments
	std::vector<segment> remap;
	for(auto s : shard)
	{
		if(segRev.count(s.sid) == 0) { remap.push_back(s); }
	}
	// std::cout << "UNMAPPED SHARDS " << remap.size() << std::endl;
	std::vector<Segment> ret;
	// Remap all unmapped segments
	for(auto r : remap)
	{
		ret.push_back(cache(ws, gidgen(), r.sid, height));
	}
	return ret;
}

void Layer::updateConstraint(Segment seg, std::vector<Constraint> con)
{
	uint32_t sid = segMap[seg.sid];
	constraint[sid] = con;
}

// Workspace class definitions
Workspace::Workspace(cairo_surface_t* can, std::vector<Vertex> boundary, double scale)
{
	// Store the canvas
	const_canvas = can;
	const_scale = scale;
	// Create a random lambda that avoids the GODAMN BOILERPLATE!
	// Because random is statefull we need a mutable tab
	// This is bonkers but at least you CAN do bonkers stuff in C++
	std::default_random_engine gen;
	std::uniform_real_distribution<double> dis(0.0, 1.0);
	auto S = std::chrono::system_clock::now().time_since_epoch().count();
	std::cout << "SEED" << S << std::endl;
	uint64_t seed = S;
	//uint64_t seed = 1675128892961642292;
	//uint64_t seed = 1675709149732672750;
	gen.seed(seed);
	rand = [=]() mutable -> double { return dis(gen); };
	// Setup the background layer
	background = addLayer(0,boundary);
	// Ensure that we are immediately ready for all operations
	ensureReadyLayout();
	// ensureReadyRender(); TODO: organization
	// Setup the most basic constraints
}

// Copy constructor
Workspace::Workspace(const Workspace& base, cairo_surface_t* can)
{
	const_canvas = can;
	const_scale = base.const_scale;
	constraint = base.constraint;
	oper = base.oper;
	brush = base.brush;
	// Create a random lambda that avoids the GODAMN BOILERPLATE!
	// Because random is statefull we need a mutable tab
	// This is bonkers but at least you CAN do bonkers stuff in C++
	std::default_random_engine gen;
	std::uniform_real_distribution<double> dis(0.0, 1.0);
	auto S = std::chrono::system_clock::now().time_since_epoch().count();
	std::cout << "SEED" << S << std::endl;
	uint64_t seed = S;
	//uint64_t seed = 1675128892961642292;
	//uint64_t seed = 1675709149732672750;
	gen.seed(seed);
	rand = [=]() mutable -> double { return dis(gen); };
	// Add all layers
	height = base.height;
	background = base.background;
	/*
	for(auto it = base.layer.begin(); it != base.layer.end(); ++it)
	{
		layer[it->first] = it->second;
		if(it->first == 0) { background = it->second; }
	}*/
	logic = base.logic;
	// Ensure we are ready
	ensureReadyLayout();
}

Layer* Workspace::addLayer(uint32_t h, std::vector<Vertex> boundary)
{
	Layer* ptr = new Layer(boundary);
	assert(layer.count(h) == 0);
	// Keep the heights sorted
	auto i = std::upper_bound(height.begin(), height.end(), h);
	height.insert(i,h);
	// Store the layer
	layer[h] = ptr;
	return ptr;
}

bool Workspace::ensureReadyLayout()
{
	// Clear previous segment cache and recache everything
	segment.clear();
	// Clear previous link map cache
	linkMap.clear();
	// Recache everything
	for(auto & [h,l] : layer)
	{
		for(auto s : l->recache(this, h, sidGen()))
		{
			segment.push_back(s);
		}	
	}
	// Link all the segments
	for(auto s : segment) { linkMap[s.sid] = {}; }
	for(auto & [h,l] : layer)
	{
		for(auto link : logic)
		{
			for(auto sid : l->logic(link))
			{
				linkMap[sid].insert(link);
			}
		}
	}
	return true;
}

double Workspace::layoutStep(std::vector<double> zipfs)
{
	if(!ensureReadyLayout()) { return false; };
	// Find the best operator
	std::vector<Callback> cand;
	uint32_t z = 0;
	for(auto o : oper)
	{
		Callback layout = o.layout(this, o);
		layout.match *= zipfs[z]; // zipf's weighting!
		cand.push_back(layout);
		/*
		std::cout << "OPERATOR " << o.name
			<< " " << layout.usable
			<< " " << layout.match
			<< " " << layout.priority << std::endl;
		//*/
		z++;
	}
	// std::cout << "CAND " << cand.size() << std::endl;
	// Pre breaking condition(s)
	if(cand.size() == 0) { return 0.0; }
	// Weighted choice only by match
	Callback cb = weighted_choice(this, cand, 0.0);
	// Run the operator if possible
	if(cb.usable) { cb.callback(); }
	// Find the (new) threshold
	double max = 0.0;
	for(auto c : cand) { max = c.match > max ? c.match : max; }
	// printf("\nNew Threshold: %f",max);
	return max;
}

bool Workspace::addBrush(Brush b) { brush.push_back(b); return true; }
bool Workspace::addOperator(Operator op) { oper.push_back(op); return true; }
bool Workspace::addConstraint(Constraint con)
{
	constraint.push_back(con);
	printf("ADDING CONSTRAINT %s, %d, %f\n",con.name.c_str(), con.mask, con.dial);
	// Distribute the constraint to all segments in all layers
	for(auto s : cut())
	{
		Layer* l = layer[s.layer];
		std::vector<Constraint> cons = s.constraint;
		cons.push_back(con);
		l->updateConstraint(s, cons);
	}
	// Update caches to perserve the change
	ensureReadyLayout();
	return true;
}


cairo_surface_t* makeSurface(std::string filename)
{
	cairo_surface_t* surface;
	surface = cairo_svg_surface_create(filename.c_str(),1920,1080);
	cairo_svg_surface_restrict_to_version(surface, CAIRO_SVG_VERSION_1_2);
	cairo_svg_surface_set_document_unit(surface, CAIRO_SVG_UNIT_PX);
	return surface;
}

bool Workspace::runTempere(uint32_t steps,bool debug)
{
	// Run a certain amount of layout steps
	// Zipf's weighting for operators
	std::vector<double> zipfs = zipfs_weight(this, oper.size());
	
	double threshold = -1.0;
	double min = -1.0;
	uint32_t step = 0;
	while(threshold == -1.0 || min == -1.0 || min < threshold)
	{
		// Break at the hardcoded step limit
		if(!(steps == (uint32_t)-1) && step >= steps) { break; }
		else { step += 1; }
		// Otherwise, layout the picture one step at a time
		threshold = layoutStep(zipfs);
		min = min == -1 ? 0.0 : min + 0.01;
		// If we are in debug code, render step
		if(debug) {
			printf("Stop values %f,%f\n",min,threshold);
			// Make a snapshot image
			std::string output = "./debug/DebugStep" + std::to_string(step) + ".svg";
			printf(output.c_str());
			cairo_surface_t* srfc = makeSurface(output);
			Workspace* snap = new Workspace(*this,srfc);
			snap->renderDebug();
		}
	}
	return true;
}

std::vector<Segment> Workspace::cut() { return segment; }
std::set<Segment> Workspace::geomRel(Segment s)
{
	std::set<Segment> ret;
	for(auto sid : layer[s.layer]->geom(s)) { ret.insert(segment[sid]); }
	return ret;
}
std::set<Segment> Workspace::logicRel(Segment s)
{
	std::set<Segment> out;
	// All the links from this segment
	for(auto link : linkMap[s.sid])
	{
		// Find all the segmentids from these links
		for(auto & [h,l] : layer)
		{
			for(auto sid : l->logic(link))
			{
				out.insert(segment[sid]);
			}
		}
	}
	return out;
}

void Workspace::linkSegment(Operator op, Segment head, Segment tail)
{
	// Check if operator is allowed TODO: this
	for(auto o : oper) { if(o.name == op.name) { printf(" "); } }
	// Check if there is a link between the two already
	auto H = linkMap[head.sid];
	auto T = linkMap[tail.sid];
	std::set<uint32_t> I;
	std::set_intersection(
		H.begin(),H.end(),
		T.begin(),T.end(),
		std::inserter(I,I.begin()));
	if(I.size() > 0) { return; }
	// Otherwise just make a new link
	uint32_t newlid = logic.size();
	logic.push_back(newlid);
	layer[head.layer]->linkLogical(head,newlid);
	layer[tail.layer]->linkLogical(tail,newlid);
}

void Workspace::addSegment(
	Operator op,
	uint32_t startlayer,
	std::vector<Vertex> bound,
	uint32_t mark)
{
	// Test if the segment is (fully, open set) within another
	auto checkbounce = [=](uint32_t lid) -> bool
	{
		for(auto seg : segment)
		{
			// We are looking for an open interior except for point
			bool open = bound.size() < 2;
			bool inter = interior(seg.boundary,bound,open);
			if(seg.layer == lid && inter) { return true; }
		}
		return false;
	};
	// Get the layer id by bouncing, potentially multiple times
	uint32_t lid;
	for(lid = startlayer; checkbounce(lid); lid++);
	// Create a new layer if needed, with just the added segment
	if(!layer.count(lid))
	{
		// printf("New layer with lid %i\n",lid);
		layer[lid] = new Layer(bound);
		return;
	}
	// If there is already a layer here, we must do tempere on the layer
	layer[lid]->tempere(bound);
	// Mark the created segment(s) and recache them
	for(auto s : layer[lid]->unmappedSegment(this,lid,sidGen()))
	{
		op_cache[op][s] = mark;
	}
	// Then recache them
	// layer[lid]->recache(this, lid, sidGen());
}

void Workspace::setConstraint(
	Operator op, Segment seg, std::vector<Constraint> con)
{
	// Update the constraints on the correct layer
	uint32_t lid = seg.layer;
	Layer* lay = layer[lid];
	lay->updateConstraint(seg, con);
}

bool Workspace::ensureReadyRender()
{
	// A gid generator
	printf("\nREADYING RENDER\n");
	// Ensure all segments are cached correctly
	if(!ensureReadyLayout()) { return false; }
	return true;
}

std::vector<Callback> Workspace::drawSegment(Segment s, std::vector<double> z)
{
	// Arrange candidates in priority order
	std::vector<Callback> cand;
	uint32_t zid = 0;
	for(auto b : brush)
	{
		Callback draw = b.draw(this, s, b);
		draw.match *= z[zid]; // zipf's weighting!
		cand.push_back(draw);
		zid++;
	}
	// The callbacks to completed drawing lambdas
	std::vector<Callback> ret;
	// Breaking condition
	if(cand.size() == 0) { return ret; }
	// Weighted choice only by match
	Callback cb = weighted_choice(this, cand, 0.0);
	if(!cb.usable) { return ret; }
	ret.push_back(cb);
	// Additional draws weighted by priority
	// TODO: allow brushes to indicate a probability of being done after
	// running once. This will allow multiple brushes on the same segment
	// while also allowing for sorting of brushstrokes by priority.
	double adjust = 1.0;
	double r = rand();
	uint32_t drawnum = 0;
	while(r < ((1.0-cb.priority) * adjust))
	{
		// printf("Rand %f\n",r);
		cb = weighted_choice(this, cand, adjust);
		// printf("Usable %s\n",cb.usable ? "true" : "false");
		if(!cb.usable) { break; }
		ret.push_back(cb);
		adjust *= 0.95;
		r = rand();
		drawnum++;
	}
	return ret;
}

bool Workspace::renderDebug()
{
	printf("Entering debug render");
	if(!ensureReadyRender())
	{
		printf("Broken render!\n");
		return false;
	} 
	// Draw the background in solid colors, with border
	std::vector<Segment> seg = cut();
	printf("There are %i total segments",seg.size());
	for(auto s : seg)
	{
		if(s.layer == 0)
		{
			// printf("BACKGROUND SEGMENT %d\n",s.sid);
			solid_brush.draw(this,s,solid_brush).callback();
		}
	}
	// Draw the logical connections
	// TODO: implement
	// Draw the local connections
	// TODO: implement
}

bool Workspace::render()
{
	// Ensure all layers are segmented properly
	if(!ensureReadyRender()) { return false;  }
	// Draw all the segments!
	std::vector<Segment> seg = cut();
	// Draw the background in solid colors
	for(auto s : seg)
	{
		if(s.layer == 0)
		{
			// printf("BACKGROUND SEGMENT %d\n",s.sid);
			solid_brush.draw(this,s,solid_brush).callback();
		}
	}
	// Zipfs weighting for brushes
	std::vector<double> zipfs = zipfs_weight(this, brush.size());
	// Draw by layer
	std::vector<Callback> frozen;
	for(auto h : height)
	{
		for(auto s : seg)
		{
			if(s.layer == h && zipfs.size() > 0)
			{
				frozen.clear();
				printf("\rDrawing Segment %d...",s.sid);
				fflush(stdout);
				for(auto cb : drawSegment(s, zipfs))
				{
					insertsort(frozen, cb);
				}
				// Call drawbacks for segment in prio order
				for(auto cb : frozen) { cb.callback(); }
			}
		}
	}
	printf("\n");
	return true;
}

void init_constraints(Workspace* ws)
{
	// Constraints are found in the constraints.h file
	// We want a better way to init these
	std::vector<ConstraintFactory> constraint =
	{
		SizeFactory(),
		ComplexityFactory(),
		OrientationFactory(),
		PerturbationFactory(),
		LightingFactory(),
		// Add more stuff here
		PaletteFactory()
		
	};
	auto zipfs = zipfs_weight(ws, constraint.size());
	// Make a zipfs for each item
	for(uint32_t i = 0; i < constraint.size(); i++)
	{
		ws->addConstraint(constraint[i].create(zipfs[i],ws->rand));
	}
}


void init_workspace(Workspace* ws)
{
	// Operators are found in the operators.h file!
	std::vector<Operator> oper = 
	{
		symmetry_operator,
		//figure_and_ground_operator,
		//focal_point_operator,
		//gradient_operator
	};
	for(auto op : oper) { ws->addOperator(op); }
	// Brushes are found in the brushes.h file!
	std::vector<Brush> brush =
	{
		solid_brush,
		// shape_brush,
		// line_brush,
		specularhighlight_brush
	};
	for(auto br : brush) { ws->addBrush(br); }
	// Initialize the constraints
	init_constraints(ws);
}

// TODO: binary search (probably last optimization to do xD)
Callback weighted_choice(Workspace* ws, std::vector<Callback> cbs, double d)
{
	// Weight the choice by how well it matches the workspace
	auto weight = [=](Callback c) -> double
	{
		return (c.match);
	};
	std::vector<Callback> cand;
	for(auto cb : cbs)
	{
		//std::cout << cb.priority << " " << weight(cb) << std::endl;
		if(cb.usable && (weight(cb) > d)) { cand.push_back(cb); }
	}
	// std::cout << "CLEANED CANDIDATES " << cand.size() << std::endl;
	double sum = 0.0;
	for(auto c : cand) { sum += weight(c); }
	double pick = sum * ws->rand();
	for(auto c : cand)
	{
		pick -= weight(c);
		if(pick <= 0) { return c; }
	}
	return {false, 0.0, 0.0, NULL};
}

void save_picture(cairo_surface_t* surface, std::string filename)
{
	printf("Saving to %s\n",filename.c_str());
	cairo_surface_finish(surface);
	cairo_surface_destroy(surface);
	// cairo_surface_write_to_png(svg,filename.c_str());
	// cairo_surgace_write_to_svg(canvas,filename.c_str());
}

#ifdef TEST_RENDER
void test_render(std::string filename, bool debug)
{
	// The beggining boundary is just all edges!
	// Default aspect ratio is 16:9, or 1920 x 1080
	cairo_surface_t* surface = makeSurface(filename);
	std::vector<Vertex> boundary = {
		{0.0,0.0},
		{16.0,0.0},
		{16.0,9.0},
		{0.0,9.0}};
	double scale = 120.0;
	// TODO: maybe resolution choice?
	Workspace* draft = new Workspace(surface,boundary,scale);
	// Initialize operators and brushes
	init_workspace(draft);
	// Run the tempere algorithm to completion
	//draft->runTempere(-1);
	//draft->runTempere(8);
	//draft->runTempere(6);
	draft->runTempere(110,debug);
	// Render the picture to a canvas
	draft->render();
	// Save the picture to a file.
	save_picture(surface, filename);
	// A happy little message that everything is fine :)
	printf("Hello World~\n");
}

int main(int argc, char* argv[])
{
	std::string filename = "image.svg";
	bool debug = false;
	int arg = 0;
	while((arg = getopt(argc, argv, "gf:")) != -1)
	{
		switch(arg)
		{
			case 'f':
				filename = optarg;
				break;
			case 'g':
				debug = true;
				break;
			default:
				continue;
		}
	}
	test_render(filename,debug);
	return 0;
}
#endif
