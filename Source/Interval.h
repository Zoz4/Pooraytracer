#pragma once

#include <limits>

namespace Pooraytracer {


	class Interval {
	public:
		double min, max;
		Interval() :min(+std::numeric_limits<double>::infinity()), max(-std::numeric_limits<double>::infinity()) {}
		Interval(double min, double max) : min(min), max(max) {}
		Interval(const Interval& a, const Interval& b) {
			min = a.min <= b.min ? a.min : b.min;
			max = a.max >= b.max ? a.max : b.max;
		}

		double Length() const {
			return max - min;
		}
		bool Contains(double x) const {
			return min <= x && x <= max;
		}
		bool Surrounds(double x) const {
			return min < x && x < max;
		}
		double Clamp(double x) const {
			if (x < min) return min;
			if (x > max) return max;
			return x;
		}

		Interval Expand(double delta) const {
			double padding = delta / 2.;
			return Interval(min - padding, max + padding);
		}

		static const Interval empty, universe;
	};

	Interval operator+(const Interval& ival, double displacement);
	Interval operator+(double displacement, const Interval& ival);
}