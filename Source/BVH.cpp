#include "BVH.h"
#include <algorithm>
#include "Logger.h"

namespace Pooraytracer {
	BVHNode::BVHNode(HittableList list) : BVHNode(list.objects, 0, list.objects.size()) {}
	BVHNode::BVHNode(std::vector<shared_ptr<Hittable>>& objects, size_t start, size_t end)
	{
		// Build the bounding box of the span of source objects.
		bbox = AABB::empty;

		for (size_t objectIdx = start; objectIdx < end; ++objectIdx)
		{
			bbox = AABB(bbox, objects[objectIdx]->BoundingBox());
		}
		int axis = bbox.LongestAxis();

		auto comparator = (axis == 0) ? BoxAxisXCompare : (axis == 1) ? BoxAxisYCompare : BoxAxisZCompare;

		size_t objectSpan = end - start;
		if (objectSpan == 1) {
			left = right = objects[start];
		}
		else if (objectSpan == 2) {
			left = objects[start];
			right = objects[start + 1];
		}
		else {
			std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);
			auto mid = start + objectSpan / 2;
			left = make_shared<BVHNode>(objects, start, mid);
			right = make_shared<BVHNode>(objects, mid, end);
		}
	}
	BVHNode::BVHNode(shared_ptr<Mesh> mesh) :BVHNode(mesh->objects, 0, mesh->objects.size()) {}

	bool BVHNode::Hit(const Ray& ray, Interval domain, HitRecord& record) const
	{
		if (!bbox.Hit(ray, domain))
		{
			return false;
		}
		bool bHitLeft = left->Hit(ray, domain, record);
		bool bHitRight = right->Hit(ray, Interval(domain.min, bHitLeft ? record.time : domain.max), record);

		return bHitLeft || bHitRight;
	}
	bool BVHNode::BoxCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b, int axisIdx)
	{
		auto aAxisInterval = a->BoundingBox().GetAxisInterval(axisIdx);
		auto bAxisInterval = b->BoundingBox().GetAxisInterval(axisIdx);
		return aAxisInterval.min < bAxisInterval.min;
	}
	bool BVHNode::BoxAxisXCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b)
	{
		return BoxCompare(a, b, 0);
	}
	bool BVHNode::BoxAxisYCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b)
	{
		return BoxCompare(a, b, 1);
	}
	bool BVHNode::BoxAxisZCompare(const shared_ptr<Hittable> a, const shared_ptr<Hittable> b)
	{
		return BoxCompare(a, b, 2);
	}
}