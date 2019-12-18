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
	// index = indexes of primitives that belonged to this part of tree
	BVHNode* BVHAccel::build_tree(const std::vector<Primitive *> & _primitives, std::vector<int> & index, size_t max_leaf_size) {
		
		int B = 16; // number of regions to split
		
		// create root node with all primitives
		BBox bb;
		for (int i : index) bb.expand(primitives[i]->get_bbox());
		root = new BVHNode(bb, index[0], index.size());

		// check for ending condition
		if (index.size() <= max_leaf_size) {
			bool is_consecutive = true;
			for (int i = 1; i < index.size(); i++) {
				if (index[i] - index[i - 1] != 1) {
					is_consecutive = false;
					break;
				}
			}
			if (is_consecutive) return root;
		}

		// if not leaf node, start spliting the space using 
		// method discussed in lecture 15
		for (int direction = 0; direction < 3; direction++) {
			std::vector<std::vector<int>> bucket_index(B);
			std::vector<BBox> bucket_box(B);

			// split bbox into B buckets
			double min_coord = bb.min[direction];
			double max_coord = bb.max[direction];
			double interval = (max_coord - min_coord) / B;

			// insert every primitive into specific bucket at 
			// each axis
			int bucket;
			Vector3D centroid;
			BBox temp_box;
			for (int i : index) {
				temp_box = _primitives[i]->get_bbox();
				centroid = temp_box.centroid();
				bucket = int(centroid[direction] / interval);
				bucket_index[bucket].push_back(i);
				bucket_box[bucket].expand(temp_box);
			}

			// For each of the B-1 possible partitioning planes 
			// that cuts the B number of buckets, evaluate SAH to find
			// the best split
			int best_split = 0;
			int best_cost = INF_D;
			double SN = bb.surface_area(); // total surface area
			double SA,SB; // surface area of two splits
			int NA, NB; // number of primitives in the two region
			double Ctrav = 0.0; // cost of traversing into next branch
			double Cisect = 1.0; // cost of ray-primitive intersection
			for (int b = 1; b < B; b++) {

			}
		}
		return root;
	}

	BVHAccel::BVHAccel(const std::vector<Primitive *> &_primitives,
		size_t max_leaf_size) {
		this->primitives = _primitives;

		// TODO (PathTracer):
		// Construct a BVH from the given vector of primitives and maximum leaf
		// size configuration. The starter code build a BVH aggregate with a
		// single leaf node (which is also the root) that encloses all the
		// primitives.

		std::vector<int> index(primitives.size());
		for (int i = 0; i < primitives.size(); i++) index[i] = i;
		root = build_tree(_primitives, index, max_leaf_size);
	}


BVHAccel::~BVHAccel() {
  // TODO (PathTracer):
  // Implement a proper destructor for your BVH accelerator aggregate

}

BBox BVHAccel::get_bbox() const { return root->bb; }

bool BVHAccel::intersect(const Ray &ray) const {
  // TODO (PathTracer):
  // Implement ray - bvh aggregate intersection test. A ray intersects
  // with a BVH aggregate if and only if it intersects a primitive in
  // the BVH that is not an aggregate.

  bool hit = false;
  for (size_t p = 0; p < primitives.size(); ++p) {
    if (primitives[p]->intersect(ray)) hit = true;
  }

  return hit;
}

bool BVHAccel::intersect(const Ray &ray, Intersection *isect) const {
  // TODO (PathTracer):
  // Implement ray - bvh aggregate intersection test. A ray intersects
  // with a BVH aggregate if and only if it intersects a primitive in
  // the BVH that is not an aggregate. When an intersection does happen.
  // You should store the non-aggregate primitive in the intersection data
  // and not the BVH aggregate itself.

  bool hit = false;
  for (size_t p = 0; p < primitives.size(); ++p) {
    if (primitives[p]->intersect(ray, isect)) hit = true;
  }

  return hit;
}

}  // namespace StaticScene
}  // namespace CMU462
