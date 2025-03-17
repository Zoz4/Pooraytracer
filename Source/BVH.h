#pragma once
#include "AABB.h"
#include "Hittable.h"
#include "HittableList.h"
#include "Triangle.h"
#include "Model.h"


namespace Pooraytracer {

	class BVHNode :public Hittable {
	public:
		BVHNode(HittableList list);
		BVHNode(shared_ptr<Mesh> mesh);
		BVHNode(std::vector<shared_ptr<Hittable>>& objects, size_t start, size_t end);
		bool Hit(const Ray& ray, Interval domain, HitRecord& record) const override;
		AABB BoundingBox() const { return bbox; }

	public:
		AABB bbox;

	private:
		shared_ptr<Hittable> left;
		shared_ptr<Hittable> right;
		static bool BoxCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b, int axisIdx);
		static bool BoxAxisXCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b);
		static bool BoxAxisYCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b);
		static bool BoxAxisZCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b);

	};

}