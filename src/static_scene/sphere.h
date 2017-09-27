#ifndef CMU462_STATICSCENE_SPHERE_H
#define CMU462_STATICSCENE_SPHERE_H

#include "object.h"
#include "primitive.h"

namespace CMU462 {
namespace StaticScene {

/**
 * A sphere from a sphere object.
 * To be consistent with the triangle interface, each sphere primitive is
 * encapsulated in a sphere object. The have exactly the same origin and
 * radius. The sphere primitive may refer back to the sphere object for
 * other information such as surface material.
 */
class Sphere : public Primitive {
 public:
  /**
   * Parameterized Constructor.
   * Construct a sphere with given origin & radius.
   */
  Sphere(const SphereObject* object, const Vector3D& o, double r)
      : object(object), o(o), r(r), r2(r * r) {}

  /**
   * Get the world space bounding box of the sphere.
   * \return world space bounding box of the sphere
   */
  BBox get_bbox() const {
    return BBox(o - Vector3D(r, r, r), o + Vector3D(r, r, r));
  }

  /**
   * Ray - Sphere intersection.
   * Check if the given ray intersects with the sphere, no intersection
   * information is stored.
   * \param r ray to test intersection with
   * \return true if the given ray intersects with the sphere,
             false otherwise
   */
  bool intersect(const Ray& r) const;

  /**
   * Ray - Sphere intersection 2.
   * Check if the given ray intersects with the sphere, if so, the input
   * intersection data is updated to contain intersection information for the
   * point of intersection.
   * \param r ray to test intersection with
   * \param i address to store intersection info
   * \return true if the given ray intersects with the sphere,
             false otherwise
   */
  bool intersect(const Ray& r, Intersection* i) const;

  /**
   * Get BSDF.
   * In the case of a sphere, the surface material BSDF is stored in
   * its sphere object wrapper.
   */
  BSDF* get_bsdf() const { return object->get_bsdf(); }

  /**
   * Compute the normal at a point of intersection.
   * NOTE: This is required for all scene objects but we only need it
   * during shading so it's not made part of IntersectInfo.
   * \param p point of intersection. Note that this assumes a valid point of
   *          intersection and does not check if it's actually on the sphere
   * \return normal at the given point of intersection
   */
  Vector3D normal(Vector3D p) const { return (p - o).unit(); }

  /**
   * Draw with OpenGL (for visualizer)
   */
  void draw(const Color& c) const;

  /**
  * Draw outline with OpenGL (for visualizer)
  */
  void drawOutline(const Color& c) const;

 private:
  /**
   * Tests for ray-sphere intersection, returning true if there are
   * intersections and writing the smaller of the two intersection times in t1
   * and the larger in t2.
   */
  bool test(const Ray& ray, double& t1, double& t2) const;

  const SphereObject* object;  ///< pointer to the sphere object

  Vector3D o;  ///< origin of the sphere
  double r;    ///< radius
  double r2;   ///< radius squared

};  // class Sphere

}  // namespace StaticScene
}  // namespace CMU462

#endif  // CMU462_STATICSCENE_SPHERE_H
