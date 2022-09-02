// C++ imports
#include <stdexcept>
#include <vector>
#include <map>

// Module imports
#include "render.h"
#include "constraints.h"

Constraint size(double size)
{
	Constraint con {"size", uint32_t(DISTANCE::CLOSE), size};
	return con;
}

Constraint orientation(double orient)
{
	Constraint con {"orientation", uint32_t(DISTANCE::CLOSE), orient};
	return con;
}

Constraint complexity(double complexity)
{
	Constraint con {"complexity", uint32_t(DISTANCE::CLOSE), complexity};
	return con;
}

Constraint perturbation(double delta)
{
	Constraint con {"perturbation", uint32_t(DISTANCE::CLOSE), delta};
	return con;
}

double accumulate_dial(double dial, double next)
{
	if(dial == -1.0) { return next; }
	if(next == -1.0) { return dial; }
	else { return (dial + next) / 2.0; }
}

std::vector<Match> match_constraint(
	std::map<std::string,uint32_t> map, std::vector<Constraint> cons)
{
	std::vector<Match> ret;
	int i = 0;
	for(auto x : cons)
	{
		int type = -1;
		try { type = map.at(x.name); }
		catch (const std::out_of_range &e) { i++; continue; }
		ret.push_back({i, type, x.dial, x.mask});
		i++;
	}
	return ret;
}

double match_accumulate_dial(
	uint32_t type,
	std::map<std::string,uint32_t> map,
	std::vector<Constraint> cons)
{
	double dial = -1.0;
	for(auto m : match_constraint(map, cons))
	{
		if(m.type == type) { dial = accumulate_dial(dial, m.dial); }
	}
	return dial;
}
