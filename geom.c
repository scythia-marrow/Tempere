#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "geom.h"

Vertex add(Vertex a, Vertex b)
{
	return {a.x + b.x, a.y + b.y};
}

Vertex scale(Vertex a, double scale)
{
	return {a.x * scale, a.y * scale};
}

Vertex normalize(Vertex a)
{
	Vertex origin {0.0, 0.0};
	double len = l2_norm(origin, a);
	return scale(a, 1.0 / len);
}

Vertex vec(Vertex a, Vertex b)
{
	return {b.x - a.x, b.y - a.y};
}

bool eq(Vertex a, Vertex b)
{
	return (a.x == b.x && a.y == b.y);
}

double magnitude(Vertex a)
{
	return sqrt(a.x * a.x + a.y * a.y);
}

double angle(Vertex a, Vertex b)
{
	double mag_sum = magnitude(a) * magnitude(b);
	double dot_sum = dot(a,b);
	double quot = dot_sum / mag_sum;
	return acos(quot);
}

double cross(Vertex a, Vertex b)
{
	return a.x * b.y - a.y * b.x;
}

double dot(Vertex a, Vertex b)
{
	return a.x * b.x + a.y * b.y;
}

double l2_norm(Vertex a, Vertex b)
{
	double x = b.x - a.x;
	double y = b.y - a.y;
	return sqrt(x*x + y*y);
}

double perimeter(std::vector<Vertex> vertexes)
{
	//printf("Vertexes: %i", vertexes.size());
	if(vertexes.size() < 2) { return 0.0; }
	double perim = l2_norm(vertexes[0], vertexes[1]);
	perim += l2_norm(vertexes[vertexes.size() - 1], vertexes[0]);
	for(unsigned int v = 1; v < vertexes.size() - 1; v++)
	{
		perim += l2_norm(vertexes[v], vertexes[v + 1]);
	}
	return perim;
}

double signed_area(std::vector<Vertex> point)
{
	double area_s = 0.0;
	int size = point.size();
	for(int h = 0; h < size; h++)
	{
		int t = (size + h + 1) % size;
		area_s += cross(point[h],point[t]);
	}
	return area_s / 2.0;
}

Vertex midpoint(std::vector<Vertex> point)
{
	int size = point.size();
	Vertex vec {0.0, 0.0};
	for(auto v : point)
	{
		vec = add(vec, v);
	}
	vec = scale(vec, (1.0 / size));
	return vec;
}

Vertex centroid(std::vector<Vertex> point)
{
	Vertex centroid {0.0 , 0.0};
	int size = point.size();
	double area_s = signed_area(point);
	for(int h = 0; h < size; h++)
	{
		int t = (size + h + 1) % size;
		double a = cross(point[h], point[t]);
		centroid.x += (point[h].x + point[t].x) * a;
		centroid.y += (point[h].y + point[t].y) * a;
	}

	centroid.x /= (6.0 * area_s);
	centroid.y /= (6.0 * area_s);

	//printf("Centroid: (%f,%f)\n", centroid.x, centroid.y);
	return centroid;
}

Vertex intersect_ray_line(Vertex origin, Vertex dir, Vertex v1, Vertex v2)
{
	Vertex p1 = add(origin, scale(v1, -1.0));
	Vertex p2 = add(v2, scale(v1, -1.0));
	Vertex p3 {-1.0f * dir.y, dir.x};

	double D = dot(p2,p3);
	if(abs(D) < 0.00001) { return origin; }
	
	double t1 = cross(p2, p1) / D;
	double t2 = dot(p1,p3) / D;


	Vertex returner;
	if (t1 >= 0.0 && (t2 >= 0.0 && t2 <= 1.0))
	{
		returner = add(origin, scale(dir, t1));
	} else
	{
		returner = origin;
	}

	return returner;
}

std::vector<Vertex> intersect_ray_poly(
	Vertex o, Vertex dir, std::vector<Vertex> poly)
{
	std::vector<Vertex> ret;
	int size = poly.size();
	for(int h = 0; h < size; h++)
	{
		int t = (size + h + 1) % size;
		Vertex vrt = intersect_ray_line(o, dir, poly[h], poly[t]);
		if(eq(vrt,o)) { continue; }
		// TODO: MAKE A BETTER FIX!
		bool breaker = false;
		for(auto v : ret) { breaker = true; break; }
		if(breaker) { continue; }
		ret.push_back(vrt);
	}
	return ret;
}

Vertex nearest_point(Vertex o, std::vector<Vertex> vtx)
{
	Vertex ret = o;
	double dis = -1.0;
	for(auto v : vtx)
	{
		double nrm = l2_norm(o,v);
		dis = dis == -1.0 || nrm < dis ? nrm : dis;
	}
	return ret;
}

Vertex furthest_point(Vertex o, std::vector<Vertex> vtx)
{
	Vertex ret = o;
	double dis = -1.0;
	for(auto v : vtx)
	{
		double nrm = l2_norm(o,v);
		dis = dis == -1.0 || nrm > dis ? nrm : dis;
	}
	return ret;
}

uint32_t winding_number(Vertex v, std::vector<Vertex> boundary)
{
	auto is_left = [](Vertex p0, Vertex h0, Vertex t0)
	{
		return (((h0.x - p0.x) * (t0.y - p0.y))
			- ((t0.x - p0.x) * (h0.y - p0.y)));
	};
	uint32_t wn = 0;
	int size = boundary.size();
	for(int h = 0; h < size; h++)
	{
		int t = (size + h + 1) % size;
		Vertex H = boundary[h];
		Vertex T = boundary[t];
		double left = is_left(H, T, v);
		//printf("\tIsLeft: %f\n", left);
		//printf("\tH,T -- p: (%f,%f), (%f,%f), (%f,%f)",
		//	H.x, H.y, T.x, T.y, v.x, v.y);
		if((H.y <= v.y) && (T.y > v.y) && (left > 0))
		{
			wn++;
		}
		if((H.y > v.y) && (T.y <= v.y) && (left < 0))
		{
			wn--;
		}
	}
	return wn;
}
