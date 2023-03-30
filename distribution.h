#include <functional>
#include <cstdint>

#ifndef distribution_h
#define distribution_h

namespace dst
{

	enum DIST : uint32_t
	{
		DDELTA,
		GAUSSIAN,
		UNIFORM,
		BIMODAL,
		NONE
	};

	double continuous_sample(DIST, std::function<double()> &);
	uint32_t discrete_sample(DIST, uint32_t, std::function<double()> &);
};
#endif
