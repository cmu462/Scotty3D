#ifndef CMU462_STATICSCENE_AGGREGATE_H
#define CMU462_STATICSCENE_AGGREGATE_H

#include "scene.h"

namespace CMU462 {
namespace StaticScene {

/**
 * Aggregate provides an interface for grouping multiple primitives together.
 * Because Aggregate itself implements the Primitive interface, no special
 * support is required for intersection acceleration. Integrators can be
 * written as if there was just a single Primitive in the scene, checking for
 * intersections without needing to be concerned about how they are actually
 * found. It is also easier to experiment new acceleration techniques by simply
 * adding a new Aggregate.
 */
class Aggregate : public Primitive {
 public:
  // Implements Primitive //

  // NOTE:
  // There is no restriction on how an Aggregate should be implemented but
  // normally an Aggregate should keep track of the primitives that it is
  // holding together. Note that during a ray - aggregate intersection, if
  // intersection information is to be updated (intersect2), the aggregate
  // implementation should store the address of the primitive that the ray
  // intersected and not that of the aggregate itself.

  std::vector<Primitive*> primitives;  ///< primitives enclosed in the aggregate

  /**
   * Get BSDF.
   * An aggregate should not have a surface material as it is not an actual
   * primitive that we would want to render but an accelerator that we use to
   * speed up ray - primitive intersections. Therefore get_brdf should always
   * return the null pointer for aggregates.
   */
  BSDF* get_bsdf() const { return NULL; }
};

}  // namespace StaticScene
}  // namespace CMU462

#endif  // CMU462_STATICSCENE_AGGREGATE_H
