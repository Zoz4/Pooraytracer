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
			area = objects[start]->GetArea();
		}
		else if (objectSpan == 2) {
			left = objects[start];
			right = objects[start + 1];
			area = objects[start]->GetArea() + objects[start + 1]->GetArea();
		}
		else {
			std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);
			auto mid = start + objectSpan / 2;
			left = make_shared<BVHNode>(objects, start, mid);
			right = make_shared<BVHNode>(objects, mid, end);
			std::shared_ptr<BVHNode> leftNode = std::dynamic_pointer_cast<BVHNode>(left);
			std::shared_ptr<BVHNode> rightNode = std::dynamic_pointer_cast<BVHNode>(right);

			if (leftNode && rightNode) {
				area = leftNode->area + rightNode->area;
			}
			else {
				area = 0.0;
				for (size_t objectIdx = start; objectIdx < end; ++objectIdx) {
					area += objects[objectIdx]->GetArea();
				}
			}
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
	void BVHNode::Sample(const point3& origin, HitRecord& samplePointRecord, double& pdf) const
	{
		double p = std::sqrt(RandomDouble()) * GetArea();
		TraverseSample(origin, make_shared<const BVHNode>(*this), p, samplePointRecord, pdf);
		pdf /= GetArea();
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
	void BVHNode::TraverseSample(const point3& origin, const shared_ptr<const Hittable> node, float p, HitRecord& samplePointRecord, double& pdf) const {

		const shared_ptr<const BVHNode> bvhNode = std::dynamic_pointer_cast<const BVHNode>(node);
		if (!bvhNode) {
			node->Sample(origin, samplePointRecord, pdf);
			pdf *= node->GetArea();
			return;
		}
		if (p < bvhNode->left->GetArea()) {
			TraverseSample(origin, bvhNode->left, p, samplePointRecord, pdf);
		}
		else {
			TraverseSample(origin, bvhNode->right, p - bvhNode->left->GetArea(), samplePointRecord, pdf);
		}
	}
}