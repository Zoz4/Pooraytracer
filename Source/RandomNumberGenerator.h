#pragma once

#include<cstdlib>

namespace Pooraytracer {

	inline float RandomFloat() {
		// Returns a random real in [0, 1).
		return std::rand() / (RAND_MAX + 1.0f);
	}
	inline float RandomFloat(float min, float max) {
		// Returns a random real in [min,max).
		return min + (max - min) * RandomFloat();
	}
	inline int RandomInt(int min, int max) {
		// Returns a random integer in [min,max].
		return int(RandomFloat(min, max + 1));
	}

}