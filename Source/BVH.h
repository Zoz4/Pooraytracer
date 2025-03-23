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
		AABB BoundingBox() const override { return bbox; }
		double GetArea() const override { return area; }
		void Sample(const point3& origin, HitRecord& samplePointRecord, double& pdf) const override;

	public:
		AABB bbox;

	private:
		shared_ptr<Hittable> left;
		shared_ptr<Hittable> right;
		static bool BoxCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b, int axisIdx);
		static bool BoxAxisXCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b);
		static bool BoxAxisYCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b);
		static bool BoxAxisZCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b);
		void TraverseSample(const point3& origin, const shared_ptr<const Hittable> node, float p, HitRecord& samplePointRecord, double& pdf) const;
		double area = 0.0;
	};

}