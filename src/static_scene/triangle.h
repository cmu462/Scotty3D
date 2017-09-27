#ifndef CMU462_STATICSCENE_TRIANGLE_H
#define CMU462_STATICSCENE_TRIANGLE_H

#include "object.h"
#include "primitive.h"

namespace CMU462 {
namespace StaticScene {

/**
 * A single triangle from a mesh.
 * To save space, it holds a pointer back to the data in the original mesh
 * rather than holding the data itself. This means that its lifetime is tied
 * to that of the original mesh. The primitive may refer back to the mesh
 * object for other information such as normal, texcoord, material.
 */
class Triangle : public Primitive {
 public:
  /**
   * Constructor.
   * Construct a mesh triangle with the given indicies into the triangle mesh.
   * \param mesh pointer to the mesh the triangle is in
   * \param v1 index of triangle vertex in the mesh's attribute arrays
   * \param v2 index of triangle vertex in the mesh's attribute arrays
   * \param v3 index of triangle vertex in the mesh's attribute arrays
   */
  Triangle(const Mesh* mesh, vector<size_t>& v);
  Triangle(const Mesh* mesh, size_t v1, size_t v2, size_t v3);

  /**
   * Get the world space bounding box of the triangle.
   * \return world space bounding box of the triangle
   */
  BBox get_bbox() const;

  /**
   * Ray - Triangle intersection.
   * Check if the given ray intersects with the triangle, no intersection
   * information is stored.
   * \param r ray to test intersection with
   * \return true if the given ray intersects with the triangle,
             false otherwise
   */
  bool intersect(const Ray& r) const;

  /**
   * Ray - Triangle intersection 2.
   * Check if the given ray intersects with the triangle, if so, the input
   * intersection data is updated to contain intersection information for the
   * point of intersection.
   * \param r ray to test intersection with
   * \param i address to store intersection info
   * \return true if the given ray intersects with the triangle,
             false otherwise
   */
  bool intersect(const Ray& r, Intersection* i) const;

  /**
   * Get BSDF.
   * In the case of a triangle, the surface material BSDF is stored in
   * the mesh it belongs to.
   */
  BSDF* get_bsdf() const { return mesh->get_bsdf(); }

  /**
   * Draw with OpenGL (for visualizer)
   */
  void draw(const Color& c) const;

  /**
   * Draw outline with OpenGL (for visualizer)
   */
  void drawOutline(const Color& c) const;

 private:
  const Mesh* mesh;  ///< pointer to the mesh the triangle is a part of

  size_t v1;  ///< index into the mesh attribute arrays
  size_t v2;  ///< index into the mesh attribute arrays
  size_t v3;  ///< index into the mesh attribute arrays

  vector<size_t> v;

};  // class Triangle

}  // namespace StaticScene
}  // namespace CMU462

#endif  // CMU462_STATICSCENE_TRIANGLE_H
