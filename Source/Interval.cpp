#include "Interval.h"


namespace Pooraytracer {

	const Interval Interval::empty = Interval(
		+std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity()
	);
	const Interval Interval::universe = Interval(
		-std::numeric_limits<float>::infinity(),
		+std::numeric_limits<float>::infinity()
	);

	Interval operator+(const Interval& ival, float displacement)
	{
		return Interval(ival.min + displacement, ival.max + displacement);
	}
	Interval operator+(float displacement, const Interval& ival)
	{
		return ival + displacement;
	}
}