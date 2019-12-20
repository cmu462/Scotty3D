#include "bvh.h"

#include "CMU462/CMU462.h"
#include "static_scene/triangle.h"

#include <iostream>
#include <stack>

using namespace std;

namespace CMU462 {
namespace StaticScene {

	// build the BVH tree using discussed in lecture 15
	// This function is called recursively
	// returns the root of the tree/sub-tree
	// changes the order of items in _primitives
	BVHNode* BVHAccel::build_tree(std::vector<Primitive *> &_primitives, size_t start, size_t range, size_t max_leaf_size) {

		int B = 12; // number of buckets to split

		// create root node with all primitives
		BBox bb;
		for (size_t i = start; i < start + range; i++) bb.expand(_primitives[i]->get_bbox());
		BVHNode* result = new BVHNode(bb, start, range);

		// check for ending condition
		if (range <= max_leaf_size) return result;

		// if not leaf node, start spliting the space using 
		// method discussed in lecture 15
		int best_split = 0, best_direction = 0;
		double temp_cost, best_cost = INF_D;
		double SN = bb.surface_area(); // total surface area
		double SA, SB; // surface area of two splits
		int NA, NB; // number of primitives in the two region
		double Ctrav = 0.0; // cost of traversing into next branch
		double Cisect = 1.0; // cost of ray-primitive intersection
		for (int direction = 0; direction < 3; direction++) {
			std::vector<std::vector<size_t>> bucket_index(B);
			std::vector<BBox> bucket_box(B);

			// split bbox into B buckets
			double min_coord = bb.min[direction];
			double max_coord = bb.max[direction];
			double interval = (max_coord - min_coord) / B;

			// no need to split in a direction without too much variation
			if (abs(max_coord - min_coord) < 0.000001) continue;

			// insert every primitive into specific bucket 
			// in every possible direction
			int bucket;
			Vector3D centroid;
			BBox temp_box;
			for (size_t i = start; i < start + range; i++) {
				temp_box = _primitives[i]->get_bbox();
				centroid = temp_box.centroid();
				bucket = int((centroid[direction] - min_coord) / interval);
				if (bucket == B) bucket--; // prevent the centriod to be the same as max_coord
				bucket_index[bucket].push_back(i);
				bucket_box[bucket].expand(temp_box);
			}

			// For each of the B-1 possible partitioning planes 
			// that cuts the B number of buckets, evaluate SAH to find
			// the best split
			for (int b = 1; b < B; b++) {
				NA = 0;
				NB = 0;
				BBox lbox, rbox;
				for (int left = 0; left < b; left++) {
					NA += bucket_index[left].size();
					lbox.expand(bucket_box[left]);
				}
				for (int right = b; right < B; right++) {
					NB += bucket_index[right].size();
					rbox.expand(bucket_box[right]);
				}
				SA = lbox.surface_area();
				SB = rbox.surface_area();

				if (NA == 0 || NB == 0) continue;
				temp_cost = Ctrav + SA / SN * NA*Cisect + SB / SN * NB*Cisect;
				if (temp_cost < best_cost) {
					best_cost = temp_cost;
					best_split = b;
					best_direction = direction;
				}
			}
		}

		// if best partition cannot be found, i.e. all
		// primitives have the same center, return 
		// BVH with all of them in a node
		if (best_split == 0) return result;

		// otherwise partition the primitives using cut method 
		// that results in the lowest cost
		double min_coord = bb.min[best_direction];
		double max_coord = bb.max[best_direction];
		double interval = (max_coord - min_coord) / B;
		std::vector<Primitive *> left_primitive;
		std::vector<Primitive *> right_primitive;
		Vector3D centroid;
		BBox temp_box;
		int bucket;
		for (size_t i = start; i < start + range; i++) {
			temp_box = _primitives[i]->get_bbox();
			centroid = temp_box.centroid();
			bucket = int((centroid[best_direction] - min_coord) / interval);
			if (bucket >= best_split) right_primitive.push_back(_primitives[i]);
			else left_primitive.push_back(_primitives[i]);
		}

		// re-order the _primitives vector to put all primitives in
		// left and right partition together
		int left_primitive_size = left_primitive.size();
		int right_primitive_size = right_primitive.size();
		for (size_t i = 0; i < left_primitive.size(); i++) {
			_primitives[i + start] = left_primitive[i];
		}
		for (size_t i = 0; i < right_primitive.size(); i++) {
			_primitives[i + start + left_primitive_size] = right_primitive[i];
		}
		left_primitive.clear();
		right_primitive.clear();

		// call itself recursively to build the left 
		// and right subtrees
		result->l = build_tree(_primitives, start, left_primitive_size, max_leaf_size);
		result->r = build_tree(_primitives, start + left_primitive_size, right_primitive_size, max_leaf_size);
		return result;
	}

	BVHAccel::BVHAccel(const std::vector<Primitive *> &_primitives,
		size_t max_leaf_size) {
		this->primitives = _primitives;

		// TODO (PathTracer):
		// Construct a BVH from the given vector of primitives and maximum leaf
		// size configuration. The starter code build a BVH aggregate with a
		// single leaf node (which is also the root) that encloses all the
		// primitives.

		root = build_tree(this->primitives, 0, primitives.size(), max_leaf_size);
	}

	// delete the BVH tree recursively
	void BVHAccel::delete_tree(BVHNode* node) {
		if (node == NULL) return;
		delete_tree(node->l);
		delete_tree(node->r);
		delete node;
		return;
	}

	BVHAccel::~BVHAccel() {
		// TODO (PathTracer):
		// Implement a proper destructor for your BVH accelerator aggregate
		delete_tree(root);
		printf("BVH deleted!\n");
	}

BBox BVHAccel::get_bbox() const { return root->bb; }

	// called recursivly to check if the ray is intersecting
	// the given BVH node. 
bool BVHAccel::find_closest_hit(const Ray &ray, BVHNode* node) const {
	if (node == NULL) return false;

	double t0, t1;
	bool hit = node->bb.intersect(ray, t0, t1);
	if (!hit) return false;

	if (node->isLeaf()) {
		hit = false;
		for (size_t p = node->start; p < node->start + node->range; ++p) {
			hit = hit | primitives[p]->intersect(ray);
		}
		return hit;
	}
	else {
		hit = find_closest_hit(ray, node->l);
		hit = hit | find_closest_hit(ray, node->r);
		return hit;
	}
}

bool BVHAccel::intersect(const Ray &ray) const {
  // TODO (PathTracer):
  // Implement ray - bvh aggregate intersection test. A ray intersects
  // with a BVH aggregate if and only if it intersects a primitive in
  // the BVH that is not an aggregate.

	return find_closest_hit(ray, root);
}

	// called recursivly to check if the ray is intersecting
	// the given BVH node. If there is hit, return the hit information
	// to the primitive closed to the ray. Uses node visit order optimizations
bool BVHAccel::find_closest_hit(const Ray &ray, BVHNode* node, Intersection *isect) const {
	if (node->isLeaf()) {
		bool hit = false;
		for (size_t p = node->start; p < node->start + node->range; ++p) {
			hit = hit | primitives[p]->intersect(ray, isect);
		}
		return hit;
	}
	else {
		bool hit_l, hit_r;
		double t0_l, t1_l, t0_r, t1_r;
		if (node->l == NULL) hit_l = false;
		else hit_l = node->l->bb.intersect(ray, t0_l, t1_l);
		if (node->r == NULL) hit_r = false;
		else hit_r = node->r->bb.intersect(ray, t0_r, t1_r);
		
		if (!hit_l && !hit_r) return false;
		else if (!hit_l && hit_r) {
			return find_closest_hit(ray, node->r, isect);
		}
		else if (!hit_r && hit_l) {
			return find_closest_hit(ray, node->l, isect);
		}
		else {
			BVHNode* first, *second;
			double second_t;
			bool true_hit;
			if (t0_l <= t0_r) {
				first = node->l;
				second = node->r;
				second_t = t0_r;
			}
			else {
				first = node->r;
				second = node->l;
				second_t = t0_l;
			}

			true_hit = find_closest_hit(ray, first, isect);
			if (!true_hit || isect->t > second_t) {
				true_hit = true_hit | find_closest_hit(ray, second, isect);
			}
			return true_hit;
		}
	}
}

bool BVHAccel::intersect(const Ray &ray, Intersection *isect) const {
	// TODO (PathTracer):
	// Implement ray - bvh aggregate intersection test. A ray intersects
	// with a BVH aggregate if and only if it intersects a primitive in
	// the BVH that is not an aggregate. When an intersection does happen.
	// You should store the non-aggregate primitive in the intersection data
	// and not the BVH aggregate itself.
	if (root == NULL) return false;
	double t0, t1;
	bool hit = root->bb.intersect(ray, t0, t1);
	if (!hit) return false;
	else return find_closest_hit(ray, root, isect);
}

}  // namespace StaticScene
}  // namespace CMU462
