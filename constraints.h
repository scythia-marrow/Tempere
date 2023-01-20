// C++ imports
#include <vector>
#include <map>

// Module imports
#include "render.h"

#ifndef constraints_h
#define constraints_h
enum DIST : uint32_t
{
	DDELTA,
	GAUSSIAN,
	NONE // The constraint that ends, for future addition!
};

Constraint size(double size);
Constraint complexity(double complexity);
Constraint orientation(double orient);
Constraint perturbation(double delta);

typedef struct match
{
	uint32_t i;
	uint32_t type;
	uint32_t mask;
	double dial;
} Match;

std::vector<Match> match_constraint(std::string, std::vector<Constraint> cons);
std::function<double(std::function<double()>)> distribution(std::vector<Match>);
#endif
