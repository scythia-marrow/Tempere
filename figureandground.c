#include <stdio.h>
#include <math.h>

// C++ imports
#include <vector>
#include <map>

// module imports
#include "geom.h"
#include "render.h"
#include "operators.h"
#include "constraints.h"

struct dir_thunk
{
	bool dir;
	double scale;
};

const std::map<uint32_t,struct dir_thunk> thunk_map
{
	{(uint32_t)CONS::SIZE,
		{
			.dir = false,
			.scale = 0.1
		}
	},
	{(uint32_t)CONS::COMPLEXITY,
		{
			.dir = true,
			.scale = 0.3
		}
	},
};

const std::function<struct dir_thunk(uint32_t)> thunk = [](uint32_t type)
{
		try { return thunk_map.at(type); }
		catch(const std::out_of_range &e)
		{
			struct dir_thunk ret {false, 0.0};
			return ret;
		}
};

double weighted_sum(std::vector<std::pair<double, int>> v_wc)
{
	double sum_c = 0.0;
	int sum_w = 0;
	for(auto x : v_wc)
	{
		//printf("C ,W: %f, %i\n", x.first, x.second);
		sum_c += x.first * x.second;
		sum_w += x.second;
	}
	return (sum_c / sum_w);
}

uint32_t measure_fbg(Workspace* ws, Operator op, Segment s)
{
	// Back layers are background
	uint32_t min = 0;
	uint32_t max = 0;
	for(auto &s : ws->cut()) { if(s.layer > max) { max = s.layer; } }
	int l_r = max - min;
	double layer = l_r == 0 ? 0.0 : (s.layer) / (1.0 * l_r);
	// Complexity and size determines foreground and background
	double comp = match_accumulate_dial(
		CONS::COMPLEXITY, op.cons, s.constraint);
	double size = match_accumulate_dial(
		CONS::SIZE, op.cons, s.constraint);

	// Depending on the direction we evaluate dials differently
	std::function<double(double,uint32_t)> dirlambda =
	[=](double c, uint32_t type)
	{
		bool dir = thunk(type).dir;
		return dir ? c : 1.0 - c;
	};
	//printf("COMP SIZ: %f %f\n",comp, size);

	comp = dirlambda(comp, CONS::COMPLEXITY);
	size = dirlambda(size, CONS::SIZE);

	//printf("COMP SIZ: %f %f\n",comp, size);

	std::vector<std::pair<double, int>> weighted_components
	{
		{layer, 10}, // Which layer the element is in
		{comp, 8}, // The complexity of the layer
		{size, 6},
	};
	double w_sum = weighted_sum(weighted_components);
	return int(256.0 * weighted_sum(weighted_components));
}

void update_fbg_cache(Workspace* ws, Operator op)
{
	// Find matching segments!
	for(auto s : ws->cut())
	{
		uint32_t place = 0;
		if(ws->op_cache[op].count(s) >= 1)
		{
			place = ws->op_cache[op][s];
		}
		else
		{
			place = measure_fbg(ws, op, s);
			ws->op_cache[op][s] = place;
		}
	}
}

struct stats
{
	double range;
	double mean;
	double variance;
	double sd;
};

void tweak_segment(Segment s, double dis)
{
	// Tweak segment size and complexity
	// TODO: make this smarter!
	
}


// Increase contrast by nudging segments away from each other
void fbglambda(Workspace* ws, Operator op, struct stats stats)
{
	printf("FOREGROUND BACKGROUND...\n");
	// Increase contrast by moving segments away from mean
	for(auto s : ws->cut())
	{
		double measure = 1.0 * ws->op_cache[op][s];
		for(auto c : match_constraint(op.cons, s.constraint))
		{
			Constraint con = s.constraint[c.i];
			double prev = con.dial;
			// Use the matching thunk
			bool left = thunk(c.type).dir;
			double scale = thunk(c.type).scale;
			// Adjust the dial
			double del = (stats.mean - measure) / 256.0;
			del = left ? -1.0 * del : del;
			// Make sure the new dial has increased contrast
			double dis = del < 0.0 ? prev : 1.0 - prev;
			// Make a new constraint
			Constraint n {con.name,0,prev + (dis * del * scale)};
			ws->setConstraint(s,{n});
			if(c.type == CONS::COMPLEXITY) {
				//printf("STATS: %i %f %f %f %f %f %f\n", left, measure, stats.mean, del, dis, prev, con->dial);
			}
		}
		// Reset the cache
		ws->op_cache[op].erase(s);
	}
	//printf("\tFBG STS: %f %f %f %f\n", stats.range, stats.mean, stats.variance, stats.sd);
}

// The basic idea is to create connected foreground areas of high complexity
// and connected areas of background with low complexity.
Callback figureandground(Workspace* ws, Operator op)
{
	// Cache the info we need for ground manipulation
	update_fbg_cache(ws, op);
	// Is there a large enough statistic between foreground and background?
	uint32_t min = 255;
	uint32_t max = 0;
	double sum = 0.0;
	double sq_sum = 0.0;
	double n = 0.0;
	for(auto s : ws->cut())
	{
		uint32_t fbg_tendency = ws->op_cache[op][s];
		min = fbg_tendency < min ? fbg_tendency : min;
		max = fbg_tendency > max ? fbg_tendency : max;
		sum += fbg_tendency;
		sq_sum += fbg_tendency * fbg_tendency;
		n += 1.0;
	}
	double mean = sum / n;
	double variance = sq_sum / n - mean * mean;
	struct stats s = 
	{
		.range = 1.0 * (max - min),
		.mean = mean,
		.variance = variance,
		.sd = sqrt(variance)
	};

	// Large SD and range shows clear figure/ground seperation
	// So we want to apply figure/ground when there is little of it
	bool usable = s.sd > (256.0 / 40.0) ? true : false;
	double contrast =
		(s.sd / 256.0 / 4.0) * 0.5
		+ (s.range / 256.0) * 0.5;
	double match = 1.0 - contrast; // We want to drive towards seperation

	Callback ret
	{
		.usable = usable,
		.match = match,
		.priority = 1.0,
		.callback = [=]() -> void { fbglambda(ws, op, s); }
	};

	return ret;
}
