// C++ imports
#include <vector>
#include <map>

// Module imports
#include "render.h"
#include "constraints.h"

Constraint size(double size)
{
	Constraint con;
	con.name = "size";
	con.mask = uint32_t(DISTANCE::CLOSE);
	con.dial = size;
	return con;
}

Constraint orientation(double orient)
{
	Constraint con;
	con.name = "orientation";
	con.mask = uint32_t(DISTANCE::CLOSE);
	con.dial = orient;
	return con;
}

Constraint complexity(double complexity)
{
	Constraint con;
	con.name = "complexity";
	con.mask = uint32_t(DISTANCE::CLOSE);
	con.dial = complexity;
	return con;
}

Constraint perturbation(double delta)
{
	Constraint con;
	con.name = "perturbation";
	con.mask = uint32_t(DISTANCE::CLOSE);
	con.dial = delta;
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
