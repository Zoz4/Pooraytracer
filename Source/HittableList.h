#pragma once
#include "Hittable.h"

#include <memory>
#include <vector>

namespace Pooraytracer {

	using std::make_shared;
	using std::shared_ptr;

	class HittableList : public Hittable {
	public:
		std::vector<shared_ptr<Hittable>> objects;
		HittableList() = default;
		HittableList(shared_ptr<Hittable> object) { Add(object); }

		void Clear() { objects.clear(); }

		void Add(shared_ptr<Hittable> object) {
			objects.push_back(object);
			bbox = AABB(bbox, object->BoundingBox());
		}

		bool Hit(const Ray& ray, Interval domain, HitRecord& record) const override {
			HitRecord tempRecord;
			bool bHitAnything = false;

			float rightBound = domain.max;
			for (const auto& object : objects) {
				if (object->Hit(ray, Interval(domain.min, rightBound), tempRecord)) {
					bHitAnything = true;
					rightBound = tempRecord.time;
					record = tempRecord;
				}
			}
			return bHitAnything;
		}
		AABB BoundingBox() const override { return bbox; }
	public:
		AABB bbox;
	};
}