// C imports
#include <math.h>

// C++ imports
#include <stdexcept>
#include <vector>
#include <map>

// Module imports
#include "render.h"
#include "constraints.h"

Constraint size(double size)
{
	Constraint con {"size", uint32_t(DIST::DDELTA), 0, size};
	return con;
}

Constraint orientation(double orient)
{
	Constraint con {"orientation", uint32_t(DIST::DDELTA), 0, orient};
	return con;
}

Constraint complexity(double comp)
{
	Constraint con {"complexity", uint32_t(DIST::DDELTA), 0, comp};
	return con;
}

Constraint perturbation(double delta)
{
	Constraint con {"perturbation", uint32_t(DIST::DDELTA), 0, delta};
	return con;
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
		if(eq) { ret.push_back({i, x.type, x.mask, x.dial}); }
		i++;
	}
	return ret;
}
