#pragma once

#include <limits>

namespace Pooraytracer {


	class Interval {
	public:
		float min, max;
		Interval() :min(+std::numeric_limits<float>::infinity()), max(-std::numeric_limits<float>::infinity()) {}
		Interval(float min, float max) : min(min), max(max) {}
		Interval(const Interval& a, const Interval& b) {
			min = a.min <= b.min ? a.min : b.min;
			max = a.max >= b.max ? a.max : b.max;
		}

		float Length() const {
			return max - min;
		}
		bool Contains(float x) const {
			return min <= x && x <= max;
		}
		bool Surrounds(float x) const {
			return min < x && x < max;
		}
		float Clamp(float x) const {
			if (x < min) return min;
			if (x > max) return max;
			return x;
		}

		Interval Expand(float delta) const {
			float padding = delta / 2.f;
			return Interval(min - padding, max + padding);
		}

		static const Interval empty, universe;
	};

	Interval operator+(const Interval& ival, float displacement);
	Interval operator+(float displacement, const Interval& ival);
}