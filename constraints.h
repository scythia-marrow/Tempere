// C++ imports
#include <vector>
#include <map>

// Module imports
#include "render.h"

#ifndef constraints_h
#define constraints_h
enum class DISTANCE
{
	CLOSE = (1<<0),
	FAR = (1<<1),
	LESS = (1<<2),
	GREATER = (1<<3)
};

enum CONS : uint32_t
{
	SIZE,
	ORIENTATION,
	COMPLEXITY,
	PERTURBATION,
	GLUE // The constraint that ends, for future addition!
};

Constraint size(double size);
Constraint complexity(double complexity);
Constraint orientation(double orient);
Constraint perturbation(double delta);

typedef struct match
{
	uint32_t i;
	uint32_t type;
	double dial;
	uint32_t mask;
} Match;

const std::map<std::string, uint32_t> global_cons_map
{
	{"size", (uint32_t)CONS::SIZE},
	{"orientation", (uint32_t)CONS::ORIENTATION},
	{"complexity", (uint32_t)CONS::COMPLEXITY},
	{"perturbation", (uint32_t)CONS::PERTURBATION}
};

std::vector<Match> match_constraint(
	std::map<std::string,uint32_t> map, std::vector<Constraint> cons);

double accumulate_dial(double, double);
double match_accumulate_dial(
	uint32_t type,
	std::map<std::string,uint32_t> map,
	std::vector<Constraint> cons);
#endif
