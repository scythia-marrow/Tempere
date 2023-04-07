#include <math.h>

#include "distribution.h"
double clamp(double s)
{
	if(s < 0.0) { return 0.0; }
	if(s > 1.0) { return 1.0; }
}

double gaussian(std::function<double()> &r)
{
	double C = 0.2;
	double s1 = r();
	double s2 = r();
	double ret = sqrt(-2.0 * log(s1)) * sin(2.0 * M_PI * s2);
	return ret * C;
}

double dst::continuous_sample(dst::DIST type, std::function<double()> &r)
{
	switch(type)
	{
		case dst::DIST::GAUSSIAN: return clamp(gaussian(r)+0.5);
		case dst::DIST::DDELTA: return 0.5;
		case dst::DIST::UNIFORM: return r();
		case dst::DIST::BIMODAL:
		{
		bool sec = r() < 0.5 ? false : true;
		double gauss = gaussian(r);
		double sample = sec ? clamp(gauss+0.25) : clamp(gauss+0.75);
		return sample;
		}
		case dst::DIST::NONE: return 0.0;
	}
}

uint32_t dst::discrete_sample(
	dst::DIST type, uint32_t size, std::function<double()> &r)
{
	double place = continuous_sample(type, r);
	uint32_t idx = int(floor(size * place));
	return idx;
}
