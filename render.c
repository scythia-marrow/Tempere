// C imports
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

std::vector<uint32_t> Layer::geom(Segment s) { return geomRel[segRev[s.sid]]; }

bool Layer::tempere(std::vector<Vertex> boundary)
{
	// Check for coverage and intersections
	for(auto p : shard)
	{
		std::vector<Vertex> perimiter;
		for(auto v : p.vid) { perimiter.push_back(vertex[v]); }
		// Is there coverage?
	}
}

// Workspace class definitions
Workspace::Workspace(cairo_surface_t* can, std::vector<Vertex> boundary)
{
	// Store the canvas
	const_canvas = can;
	// Resolution of the canvas
	int cairoX = cairo_image_surface_get_width(can);
	int cairoY = cairo_image_surface_get_height(can);
	// X and Y scale
	double tempereX = boundary[1].x - boundary[0].x;
	double tempereY = boundary[1].y - boundary[2].y;
	// The scale difference
	const_scale = cairoX / tempereX;
	// Create a random lambda that avoids the GODAMN BOILERPLATE!
	// Because random is statefull we need a mutable tab
	// This is bonkers but at least you CAN do bonkers stuff in C++
	std::default_random_engine gen;
	std::uniform_real_distribution<double> dis(0.0, 1.0);
	gen.seed(std::chrono::system_clock::now().time_since_epoch().count());
	rand = [=]() mutable -> double { return dis(gen); };
	// Setup the root layer
	background = new Layer(boundary);
	layer[0] = background;
	height.push_back(0);
}

uint32_t Workspace::nextSegment() { return segment.size(); }

bool Workspace::layoutStep()
{
	// Collect all segments in a cache
	std::vector<Segment> seg;
	for(auto & [h,l] : layer)
	{
		// Create a generator for all these new gids
		uint32_t monoid = nextSegment();
		std::function<uint32_t()> gidgen = [=]() mutable -> uint32_t
		{
			return monoid++;
		};
		for(auto s : l->recache(this,h, gidgen)) { seg.push_back(s); }	
	}
	// for(int i = 0; i < seg.size(); i++) { segshard.push_back(&seg[i]); }
	// Zipf's weighting
	std::vector<double> zipfs = zipfs_weight(this, oper.size());
	// Find the best operator
	std::vector<Callback> cand;
	uint32_t z = 0;
	for(auto o : oper)
	{
		Callback layout = o.layout(this, o);
		layout.match *= zipfs[z]; // zipf's weighting!
		cand.push_back(layout);
		z++;
	}
	// Pre breaking condition(s)
	if(cand.size() == 0) { return false; }
	// Weighted choice
	Callback cb = weighted_choice(this, cand, 0.0);
	// Run the operator if possible
	if(cb.usable) { cb.callback(); }
	else { return false; }
}

bool Workspace::addBrush(Brush b) { brush.push_back(b); return true; }
bool Workspace::addOperator(Operator op) { oper.push_back(op); return true; }
bool Workspace::addConstraint(Constraint con)
{
	constraint.push_back(con);
	return true;
}

bool Workspace::runTempere(uint32_t steps)
{
	std::cout << "IMPLEMENT TEMPERE CODE" << std::endl;
	return false;
}

std::vector<Segment> Workspace::cut() { return segment; }

void Workspace::addSegment(uint32_t layer, std::vector<Vertex> bound)
{
	std::cout << "IMPLEMENT SEGMENT ADD CODE" << std::endl;
}

void Workspace::setConstraint(Segment, std::vector<Constraint>)
{
	std::cout << "IMPLEMENT CONSTRAINT UPDATE CODE" << std::endl;
}

bool Workspace::render()
{
	std::cout << "IMPLEMENT RENDER CODE" << std::endl;
	return false;
}

void init_workspace(Workspace* ws)
{
	Constraint plt = palette((uint32_t)palette::RAND,0.0);
	Constraint cmp = complexity(0.5);
	Constraint sze = size(0.5);
	Constraint prt = perturbation(0.5); // TODO: large
	Constraint ori = orientation(0.0);
	ws->addConstraint(plt);
	ws->addConstraint(cmp);
	ws->addConstraint(sze);
	ws->addConstraint(prt);
	ws->addConstraint(ori);
	// Operators are found in the operators.h file!
	ws->addOperator(symmetry_operator);
	ws->addOperator(figure_and_ground_operator);
	ws->addOperator(focal_point_operator);
	ws->addOperator(gradient_operator);
	// Brushes are found in the brushes.h file!
	ws->addBrush(solid_brush);
	ws->addBrush(shape_brush);
	ws->addBrush(line_brush);
}

// TODO: binary search (probably last optimization to do xD)
Callback weighted_choice(Workspace* ws, std::vector<Callback> cbs, double d)
{
	// So the first choice needs to be just the match.
	// But the next needs to weight more and more by priority...
	// We weight the choice by match and priority.
	// Because the priority determines chain, we weight by its inverse
	auto weight = [=](Callback c) -> double
	{
		return (d * c.match) + ((1.0 - d) * (1.0 - c.priority));
	};
	std::vector<Callback> cand;
	for(auto cb : cbs) { if(cb.usable) { cand.push_back(cb); } }
	double sum = 0.0; for(auto c : cand) { sum += weight(c); }
	double pick = sum * ws->rand();
	for(auto c : cand)
	{
		pick -= weight(c);
		if(pick <= 0) { return c; }
	}
	return {false, 0.0, NULL};
}

/*
void layout_picture(Workspace* ws)
{
	// Zipf's weighting
	std::vector<double> zipfs = zipfs_weight(ws, ws->oper.size());
	// When the threshold for choosing an op is too high, stop
	double threshold, min = -1.0; // TODO: should be max?
	
	while(threshold == -1.0 || min == -1.0 || threshold > min)
	{
		// Find all applicable operators
		std::vector<Callback> cand;
		uint32_t z = 0;
		for(auto op : ws->)
		{
			Callback layout = op.layout(ws, op);
			layout.match *= zipfs[z]; // zipf's weighting!
			cand.push_back(layout);
			z++;
		}
		// Pre breaking condition(s)
		if(cand.size() == 0) { break; }
		// Weighted choice
		Callback cb = weighted_choice(ws, cand, 0.0);
		if(cb.usable) { cb.callback(); }
		else { break; }
		// Post breaking conditions TODO: these names are whack
		double max = 0.0;
		for(auto c : cand) { max = c.match > max ? c.match : max; }
		threshold = max;
		min = min == -1 ? 0.0 : min += 0.01;
		printf("Operator %f: %f %f\n", cb.match, min, threshold);
	}
}
*/
/*
void paint_picture(Workspace* ws)
{
	// Zipf's weighting
	std::vector<double> zipfs = zipfs_weight(ws, ws->brush.size());
	//Paint by layer
	std::map<uint64_t,std::vector<Segment*>> layer;
	for(auto s : ws->segment) { layer[s->layer].push_back(s); }
	for(int i = ws->min_layer; i <= ws->max_layer; i++)
	{
		// Paint by priority
		std::vector<Callback> frozen;
		for(auto s : layer[i])
		{
			std::vector<Callback> cand;
			uint32_t z = 0;
			for(auto b : ws->brush)
			{
				Callback draw = b.draw(ws, s, b);
				draw.match *= zipfs[z]; // zipf's weighting!
				cand.push_back(draw);
				z++;
			}
			// Breaking condition
			if(cand.size() == 0) { break; }
			// Weighted choice
			Callback cb = weighted_choice(ws, cand, 1.0);
			if(!cb.usable) { continue; }
			insertsort(frozen, cb);
			while(!(ws->rand() < cb.priority))
			{
				insertsort(frozen, cb);
				cb = weighted_choice(ws, cand, 0.5);
			}
		}
		// Perform all the callbacks in priority order
		for(auto cb : frozen) { cb.callback(); }
	}
}
*/
/*
void render_picture(Workspace* ws)
{
	// Draw a background color
	Brush back = ws->brush[0];
	back.draw(ws, ws->segment[0], back).callback();
	// Use operators and brushes to paint the picture
	layout_picture(ws);
	paint_picture(ws);
}
*/

void save_picture(cairo_surface_t* canvas, std::string filename)
{
	cairo_surface_write_to_png(canvas,filename.c_str());
}

#ifdef TEST_RENDER
void test_render(std::string filename)
{
	// The beggining boundary is just all edges!
	// Default aspect ratio is 16:9, or 1920 x 1080
	cairo_surface_t* canvas;
	std::vector<Vertex> boundary = {
		{0.0,0.0},
		{16.0,0.0},
		{16.0,9.0},
		{0.0,9.0}};
	canvas = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1920,1080);
	// TODO: maybe resolution choice?
	Workspace* draft = new Workspace(canvas,boundary);
	// Initialize all the constraints, operators, and brushes
	init_workspace(draft);
	// Run the tempere algorithm to completion
	draft->runTempere(-1);
	// Render the picture to a canvas
	draft->render();
	// Save the picture to a file.
	save_picture(canvas, filename);
	// A happy little message that everything is fine :)
	printf("Hello World~\n");
}

int main(int argc, char* argv[])
{
	std::string filename = "image.png";
	int arg = 0;
	while((arg = getopt(argc, argv, "f:")) != -1)
	{
		switch(arg)
		{
			case 'f':
				filename = optarg;
				break;
			default:
				continue;
		}
	}
	test_render(filename);
	return 0;
}
#endif
