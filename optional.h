#ifndef OPTIONAL_H
#define OPTIONAL_H
namespace opt
{
	template <typename T> struct Optional
	{
		bool is;
		T dat;
	};
};
#endif
