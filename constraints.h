// C++ imports
#include <vector>
#include <map>

// Module imports
#include "render.h"
#include "distribution.h"

#ifndef constraints_h
#define constraints_h
using namespace dst;

class ConstraintFactory
{
	protected:
		enum INITTYPE : uint32_t
		{
			MASK,
			DIAL,
			BOTH
		};
		std::string name;
		INITTYPE type;
		std::vector<DIST> dist;
		std::vector<uint32_t> mask;
	public:
		virtual Constraint create();
		virtual Constraint create(double, std::function<double()>&);
};


class SizeFactory : public ConstraintFactory
{
	public:
		SizeFactory();
};

class ComplexityFactory : public ConstraintFactory
{
	public:
		ComplexityFactory();
};

class OrientationFactory : public ConstraintFactory
{
	public:
		OrientationFactory();
};

class PerturbationFactory : public ConstraintFactory
{
	public:
		PerturbationFactory();
};

class LightingFactory : public ConstraintFactory
{
	public:
		LightingFactory();
};

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
