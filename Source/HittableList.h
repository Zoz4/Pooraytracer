#pragma once
#include "Hittable.h"
#include "RandomNumberGenerator.h"
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
			area += object->GetArea();
			bbox = AABB(bbox, object->BoundingBox());
		}

		bool Hit(const Ray& ray, Interval domain, HitRecord& record) const override {
			HitRecord tempRecord;
			bool bHitAnything = false;

			double rightBound = domain.max;
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
		double GetArea() const override {
			return area;
		}
		void Sample(const point3& origin, HitRecord& samplePointRecord, double& pdf) const override{
			double areaSum = 0.0;
			for (const auto& object : objects) {
				areaSum += object->GetArea();
			}
			double p = RandomDouble() * areaSum;

			areaSum = 0.0;
			for (const auto& object : objects) {
				areaSum += object->GetArea();
				if (p <= areaSum) {
					object->Sample(origin, samplePointRecord, pdf);
					break;
				}
			}
		}
	public:
		AABB bbox;
		double area = 0.0;
	};
}