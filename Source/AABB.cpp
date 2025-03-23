#include "AABB.h"

#include "Ray.h"

namespace Pooraytracer {

	const AABB AABB::empty = AABB(Interval::empty, Interval::empty, Interval::empty);
	const AABB AABB::universe = AABB(Interval::universe, Interval::universe, Interval::universe);

	AABB::AABB(const Interval& x, const Interval& y, const Interval& z) :
		x(x), y(y), z(z)
	{
		PadToMinimus();
	}

	AABB::AABB(const vec3& a, const vec3& b)
	{
		x = (a.x <= b.x) ? Interval(a.x, b.x) : Interval(b.x, a.x);
		y = (a.y <= b.y) ? Interval(a.y, b.y) : Interval(b.y, a.y);
		z = (a.z <= b.z) ? Interval(a.z, b.z) : Interval(b.z, a.z);
		PadToMinimus();
	}

	AABB::AABB(const AABB& box0, const AABB& box1)
	{
		x = Interval(box0.x, box1.x);
		y = Interval(box0.y, box1.y);
		z = Interval(box0.z, box1.z);
	}

	const Interval& AABB::GetAxisInterval(int axis) const
	{
		if (axis == 1) return y;
		if (axis == 2) return z;
		return x;
	}

	bool AABB::Hit(const Ray& ray, Interval t) const
	{
		const vec3& origin = ray.origin;
		const vec3& direction = ray.direction;

		for (int i = 0; i < 3; ++i) {
			const Interval& axis = GetAxisInterval(i);
			const double invDircetion = 1.0 / direction[i];

			double t0 = (axis.min - origin[i]) * invDircetion;
			double t1 = (axis.max - origin[i]) * invDircetion;

			if (t0 < t1) {
				if (t0 > t.min) t.min = t0;
				if (t1 < t.max) t.max = t1;
			}
			else {
				if (t1 > t.min) t.min = t1;
				if (t0 < t.max) t.max = t0;
			}

			if (t.max <= t.min) {
				return false;
			}
		}
		return true;
	}

	int AABB::LongestAxis() const
	{
		if (x.Length() > y.Length()) {
			return x.Length() > z.Length() ? 0 : 2;
		}
		else {
			return y.Length() > z.Length() ? 1 : 2;
		}
	}

	void AABB::PadToMinimus()
	{
		double delta = 0.0001;
		if (x.Length() < delta) x = x.Expand(delta);
		if (y.Length() < delta) y = y.Expand(delta);
		if (z.Length() < delta) z = z.Expand(delta);
	}

	AABB operator+(const AABB& bbox, const vec3& offset)
	{
		return AABB(bbox.x + offset.x, bbox.y + offset.y, bbox.z + offset.z);
	}

	AABB operator+(const vec3& offset, const AABB& bbox)
	{
		return bbox + offset;
	}

}