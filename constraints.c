// C imports
#include <math.h>

// C++ imports
#include <stdexcept>
#include <vector>
#include <map>

// Module imports
#include "render.h"
#include "constraints.h"
#include "distribution.h"

using namespace dst;

SizeFactory::SizeFactory()
{
	name = "size";
	type = INITTYPE::DIAL;
	dist = { DIST::GAUSSIAN, DIST::BIMODAL };
	mask = {};
}

ComplexityFactory::ComplexityFactory()
{
	name = "complexity";
	type = INITTYPE::DIAL;
	dist = { DIST::GAUSSIAN, DIST::BIMODAL };
	mask = {};
}

OrientationFactory::OrientationFactory()
{
	name = "orientation";
	type = INITTYPE::DIAL;
	dist = { DIST::UNIFORM };
	mask = {};
}

PerturbationFactory::PerturbationFactory()
{
	name = "perturbation";
	type = INITTYPE::DIAL;
	dist = { DIST::GAUSSIAN };
	mask = {};
}


Constraint ConstraintFactory::create()
{
	switch(type)
	{
		case INITTYPE::DIAL:
			if(dist.size() < 1)
			{
				return { name, DIST::DDELTA, (uint32_t)-1, 0.5};
			}
			return { name, dist[0], 0, 0.5 };
		break;
		case INITTYPE::MASK:
			if(mask.size() < 1)
			{
				return { name, DIST::NONE, 0, -1.0 };
			}
			return { name, DIST::NONE, mask[0], -1.0 };
		break;
		case INITTYPE::BOTH:
			if(mask.size() < 1 || dist.size() < 1)
			{
				return { name, DIST::NONE, 0, 0.5};
			}
			return { name, dist[0], mask[0], 0.5 };
		break;
	};
	return {"", DIST::NONE, (uint32_t)-1, -1.0 };
}


// Construct a random constraint given a zipfs slot and a rng
// Higher zipfs values are allowed more variation from the standard
Constraint ConstraintFactory::create(double zip, std::function<double()> &rand)
{
	auto avgdistribution = [&](double a, double b) mutable -> double
	{
		double lownum = a * rand();
		double highnum = b + (1.0 - b) * rand();
		double avgnum = lownum + (highnum - lownum) * rand();
		return avgnum;
	};
	// Select a random distance function with EV of the zipfs
	uint32_t sizdist = dist.size();
	double placedist = avgdistribution(zip,zip)*sizdist;
	uint32_t distidx = uint32_t(floor(placedist));
	// Now do the mask based on the type distribution
	uint32_t sizmask = mask.size();
	uint32_t maskidx = dst::discrete_sample(dist[distidx], sizmask, rand);
	// For the dial also base it on the type
	double dial = dst::continuous_sample(dist[distidx], rand);
	// Now return it
	switch(type)
	{
		case INITTYPE::DIAL:
			return { name, dist[distidx], 0, dial };
		break;
		case INITTYPE::MASK:
			return { name, DIST::NONE, mask[maskidx], -1.0 };
		break;
		case INITTYPE::BOTH:
			return { name, dist[distidx], mask[maskidx], dial };
		break;
	}
	return {"", DIST::NONE, (uint32_t)-1, -1.0 };
}

std::function<double(std::function<double()>)> distribution(
	std::vector<Match> match)
{
	// Choose which match to choose based on type and dial
	std::vector<Match> copy = {};
	for(auto m : match) { if(!(m.type==DIST::NONE)) { copy.push_back(m); } }
	auto zerolam = [=](std::function<double()>) -> double { return -1.0; };
	if(copy.size() == 0) { return zerolam; }
	auto ret = [=](std::function<double()> rng) mutable -> double {
		double choice = rng();
		uint32_t idx = int(floor(choice * copy.size()));
		// TODO: gaussians / distributions?
		return match[idx].dial;
	};
	return ret;
}

std::vector<Match> match_constraint(std::string s, std::vector<Constraint> cons)
{
	std::vector<Match> ret;
	uint32_t i = 0;
	for(auto x : cons)
	{
		bool eq = s.compare(x.name);
		if(eq==0) { ret.push_back({i, x.type, x.mask, x.dial}); }
		i++;
	}
	return ret;
}
