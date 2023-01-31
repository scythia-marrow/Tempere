// C imports
#include <math.h>
#include <stdio.h>

// CPP imports
#include <vector>

//Module imports
#include "../geom.h"

using geom::Vertex;
using geom::Vector;
using geom::eq;

typedef struct sample
{
	Vertex a;
	Vertex b;
	Vertex c;
	double result;
} Sample;


std::vector<double> testline(double bound, uint32_t num)
{
	std::vector<double> ret = {};
	for(uint32_t i = 0; i < num; i++)
	{
		double test = (bound * ((i + 0.0) / 2.0)) - (bound / 2.0);
		ret.push_back(test);
	}
	return ret;
}

std::vector<Vertex> testlattice(double xbound, double ybound, uint32_t num)
{
	
	std::vector<Vertex> ret = {};
	for(auto x : testline(xbound, num))
	{
		for(auto y : testline(ybound, num))
		{
			ret.push_back({x,y});
		}
	}
	return ret;
}

Vertex rotate(Vertex v, double angle)
{
	Vertex ret;
	ret.x = v.x * cos(angle) - v.y * sin(angle);
	ret.y = v.x * sin(angle) + v.y * cos(angle);
	return ret;
}

std::vector<Sample> testgen()
{
	std::vector<Sample> unrotated = {};
	// Make test sets by rotating known lines
	uint32_t numsamples = 20;
	double xbound = 1000.0;
	double ybound = 1000.0;
	// The edge is from some negative value to 0,0
	for(auto d : testline(xbound,numsamples))
	{
		Vertex a = { d - (xbound / 2.0), 0.0};
		for(auto v : testlattice(xbound, ybound, numsamples))
		{
			Vertex b = {0.0,0.0};
			Vertex c = v;
			Vector v1 = geom::vec(b,a);
			Vector v2 = geom::vec(b,c);
			double trueangle = geom::angle(v1,v2);
			if(c.y > 0.0) { trueangle = (2.0 * M_PI) - trueangle; }
			unrotated.push_back({a, {0.0,0.0}, v, trueangle});
		}
	}
	// Now, rotate each vector through a large series of angles
	std::vector<Sample> rotated = {};
	uint32_t rotations = 100;
	for(auto d : testline(M_PI*2.0,rotations))
	{
		double angle = d + M_PI;
		for(auto u : unrotated)
		{
			rotated.push_back(
			{
			rotate(u.a,angle), u.b, rotate(u.c,angle),
			u.result
			});
		}
	}
	return rotated;
}

void testrun(std::vector<Sample> testset)
{
	// Check if the directed angle function is working
	uint32_t pass = 0;
	uint32_t fail = 0;
	for(auto t : testset)
	{
		double dirangle = geom::dirangle(t.a,t.b,t.c);
		if(!eq(dirangle, t.result))
		{
			printf("TEST FAILED %f vs %f", dirangle, t.result);
			if(eq(dirangle - M_PI,t.result))
			{
				printf(" OFF BY PI(-)");
			}
			if(eq(dirangle + M_PI,t.result))
			{
				printf(" OFF BY PI(+)");
			}
			printf("\n");
			fail++;
		}
		else { pass++; }
	}
	printf("SUMMARY: %d tests, %d pass %d fail %f ratio\n",pass+fail,pass,fail, (pass+0.0)/(pass+fail));
}

int main()
{
	auto testset = testgen();
	testrun(testset);
}
