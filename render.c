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

void init_constraints(Workspace* ws, Segment* s,std::vector<Constraint> con)
{
	for(auto c : con)
	{
		Constraint nc = c;
		nc.dial = ws->rand();
		s->constraint.push_back(nc);
	}
}

// We weight operators and brushes by Zip's law to make distinct pictures with
// the same operators. Think different "languages" based on the same phonemes
std::vector<double> zipfs_weight(Workspace* ws, int N)
{
	// Make weights
	std::vector<double> weight;
	for(int i = 0; i < N; i++) { weight.push_back(1.0 / (N+1)); }
	// Shuffle them
	auto randint = [=](int i) -> int { return int(ws->rand() * i); };
	std::random_shuffle(weight.begin(), weight.end(), randint);
	return weight;
}

// Initialize the cairo context
void init_workspace(Workspace* ws)
{
	// Create a random lambda that avoids the GODAMN BOILERPLATE!
	// Because random is statefull we need a mutable tab
	// This is bonkers but at least you CAN do bonkers stuff in C++
	std::default_random_engine gen;
	std::uniform_real_distribution<double> dis(0.0, 1.0);
	gen.seed(std::chrono::system_clock::now().time_since_epoch().count());
	ws->rand = [=]() mutable -> double { return dis(gen); };
	// TODO: store constraints from previous runs and make new base ones!
	// Constraints!
	Constraint plt = palette((uint32_t)palette::RAND,0.0);
	Constraint cmp = complexity(0.5);
	Constraint sze = size(0.5);
	Constraint prt = perturbation(0.5); // TODO: large
	Constraint ori = orientation(0.0);
	ws->constraint.push_back(plt);
	ws->constraint.push_back(cmp);
	ws->constraint.push_back(sze);
	ws->constraint.push_back(prt);
	ws->constraint.push_back(ori);

	// TODO: Store a list of operators to use, such as symmetry
	//	contrast, ect!

	// Operators are found in the operators.h file!
	ws->op.push_back(symmetry_operator);
	ws->op.push_back(figure_and_ground_operator);
	ws->op.push_back(focal_point_operator);
	ws->op.push_back(gradient_operator);
	
	// Brushes are found in the brushes.h file!
	ws->brush.push_back(solid_brush);
	ws->brush.push_back(shape_brush);
	ws->brush.push_back(line_brush);

	// The root segment is what gets cut by operators and drawn on
	// by brushes
	Segment* segment = &ws->root;
	ws->segment.push_back(segment);
	// Default layer is 0, but only order matters
	segment->layer = 0;
	// Create a set of beginning constraints
	init_constraints(ws, segment, ws->constraint);
	// The beggining boundary is just all edges!
	// Default aspect ratio is 16:9, or 1920 x 1080
	segment->boundary.push_back({0.0,0.0});
	segment->boundary.push_back({16.0,0.0});
	segment->boundary.push_back({16.0,9.0});
	segment->boundary.push_back({0.0,9.0});
	// TODO: maybe resolution choice?
	segment->canvas = cairo_image_surface_create(
		CAIRO_FORMAT_ARGB32,
		1920, 1080);
	// TODO: Find scale automatically?!?!?
	segment->scale = 120;
}

// TODO: binary search (probably last optimization to do xD)
Callback weighted_choice(Workspace* ws, std::vector<Callback> cbs, double d)
{
	// So the first choice needs to be just the match. But the next needs to weight more and more by priority...
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

void insertsort(std::vector<Callback> &cb, Callback b)
{
	int i; int s = cb.size();
	for(i = 0; i < s && cb[i].priority > b.priority; i++);
	cb.insert(cb.begin() + i, b);
}

void layout_picture(Workspace* ws)
{
	// Zipf's weighting
	std::vector<double> zipfs = zipfs_weight(ws, ws->op.size());
	// When the threshold for choosing an op is too high, stop
	double threshold, min = -1.0; // TODO: should be max?
	
	while(threshold == -1.0 || min == -1.0 || threshold > min)
	{
		// Find all applicable operators
		std::vector<Callback> cand;
		uint32_t z = 0;
		for(auto op : ws->op)
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

void render_picture(Workspace* ws)
{
	// Draw a background color
	Brush back = ws->brush[0];
	back.draw(ws, ws->segment[0], back).callback();
	// Use operators and brushes to paint the picture
	layout_picture(ws);
	paint_picture(ws);
}

void save_picture(Workspace* ws, std::string filename)
{
	cairo_surface_write_to_png(ws->root.canvas,filename.c_str());
}

#ifdef TEST_RENDER
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

	Workspace draft;
	// Initialize all the constraints, operators, and brushes
	init_workspace(&draft);
	// Render the picture
	render_picture(&draft);
	// Save the picture to a file.
	save_picture(&draft, filename);
	// A happy little message that everything is fine :)
	printf("Hello World~\n");
	return 0;
}
#endif
