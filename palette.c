// C imports
#include <math.h>

// C++ imports
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <random>

// Module imports
#include "palette.h"
#include "bitutils.h"
#include "render.h"
#include "geom.h"
using namespace geom;

// TODO: REMOVE
#include <iostream>

// Single color palettes
static std::vector<std::string> white
{
	"F0F9FE", "F8F8FF", "F2F3F4", "FFFFFF"
};

static std::vector<std::string> red
{
	"750400", "950D06", "C60F00", "E50C17", "F1322C"
};

static std::vector<std::string> green
{
	"546811", "697C12", "81971C", "98AA11", "BBCD32", "D2D83F"
};

static std::vector<std::string> blue
{
	"1E90FF", "187DE9", "126AD2", "0C56BC", "0C56BC", "0643A5", "00308F"
};


Color color_hex(std::string hex)
{
	// TODO: SAFETY LMAO
	std::map<char,int> charMap
	{
	{'0', 0}, {'1',1}, {'2',2}, {'3',3}, {'4',4},
	{'5', 5}, {'6',6}, {'7',7}, {'8',8}, {'9',9},
	{'a', 10}, {'b',11}, {'c',12}, {'d',13}, {'e',14}, {'f',15},
	{'A', 10}, {'B',11}, {'C',12}, {'D',13}, {'E',14}, {'F',15}
	};
	int red = charMap[hex.c_str()[0]] * 16 + charMap[hex.c_str()[1]];
	int green = charMap[hex.c_str()[2]] * 16 + charMap[hex.c_str()[3]];
	int blue = charMap[hex.c_str()[4]] * 16 + charMap[hex.c_str()[5]];
	return
	{
		.name = std::string(hex),
		.red = (red / 255.0),
		.green = (green / 255.0),
		.blue = (blue / 255.0)
	};
}

Color color_rand(Workspace* ws)
{
	char chars[16] = {
		'0','1','2','3','4',
		'5','6','7','8','9',
		'A','B','C','D','E','F'};
	std::string hexcode = "000000";
	for(int i = 0; i < 6; i++)
	{
		int index = ws->rand() * 16.0;
		hexcode[i] = chars[index];
	}
	return color_hex(hexcode);
}

Constraint palette(uint32_t mask, double shade)
{
	Constraint con {"palette", mask, shade};
	return con;
}

Palette from_string_vecs(
	std::vector<std::string> primary,
	std::vector<std::string> complimentary,
	std::vector<std::string> triadic)
{
	Palette palette;
	for(auto str : primary)
	{
		palette.primary.push_back(color_hex(str));
	}
	for(auto str : complimentary)
	{
		palette.complimentary.push_back(color_hex(str));
	}
	for(auto str : triadic)
	{
		palette.triadic.push_back(color_hex(str));
	}
	return palette;
}

// A nice palette of red hues!
Palette reds()
{
	std::vector<std::string> primary
	{
		"c61b00", "de2d0b", "ec3511", "fa3c16", "ff431a"
	};
	std::vector<std::string> complimentary
	{
		"00a2bd", "09b7d8", "11c8ec", "2bd0f1", "52d9f5"
	};
	std::vector<std::string> triadic
	{
		"acc300", "bcd902", "c8ec11", "d0ef46", "d9f36c"
	};
	return from_string_vecs(primary, complimentary, triadic);
}

Palette trans()
{
	std::vector<std::string> primary
	{
		"ffffff", "f88bc2", "47a9fa", "ca93ca",
	};
	std::vector<std::string> complimentary
	{
		"F8BCD4", "906ACE", "ff9DC0", "7253B8", "FE63AC", "4F2685"
	};
	std::vector<std::string> triadic = white;

	return from_string_vecs(primary, complimentary, triadic);
}

// A palette of many random colors!
Palette random_palette(Workspace* ws)
{
	Palette palette;
	for(int i = 0; i < 100; i++)
	{
		palette.primary.push_back(color_rand(ws));
	}
	return palette;
}

Palette pick_palette(Workspace* ws, uint32_t mask)
{
	if(mask == 0) { return random_palette(ws); }
	// Merge palettes according to rules
	std::vector<Palette> base;

	for(auto x : BitIterator<enum palette>())
	{
		switch(mask & (uint32_t)x)
		{
			case (uint32_t)palette::RAND:
				base.push_back(random_palette(ws));
				break;
			case (uint32_t)palette::REDS:
				base.push_back(reds());
				break;
			case (uint32_t)palette::TRANS:
				base.push_back(trans());
				break;
		}
	}

	
	
	// TODO: MERGE BASE PALETTES IN A CLEVER WAY!
	return base[0];
}

Color pick_color(
	Workspace* ws,
	Palette* p,
	std::vector<Constraint> constraints)
{
	std::map<std::string,uint32_t> map {
		{"palette",CONS_PALETTE::PALETTE},
		{"perturbation",CONS::PERTURBATION}
	};
	// TODO: Make it a bit smarter yo
	double dial_p = match_accumulate_dial(
		CONS_PALETTE::PALETTE, map, constraints);
	double dial_d = match_accumulate_dial(
		CONS::PERTURBATION, map, constraints);

	// TODO: debug this!
	if(dial_d == -1.0) { dial_d = 0.5; }
	if(dial_p == -1.0) { dial_p = 0.0; }
	
	float del = dial_d;
	float del_inv = 1.0 - del;
	int place = (del_inv * dial_p + del * ws->rand()) * p->primary.size();

	// Small perturbation!
	if(place > p->primary.size() || place < 0)
	{
		return {"black", 0.0, 0.0, 0.0};
	}
	Color color = p->primary[place];
	return color;
}
