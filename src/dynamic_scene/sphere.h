#ifndef CMU462_DYNAMICSCENE_SPHERE_H
#define CMU462_DYNAMICSCENE_SPHERE_H

#include "scene.h"

#include "../collada/sphere_info.h"

namespace CMU462 {
namespace DynamicScene {

class Sphere : public SceneObject {
 public:
  Sphere(const Collada::SphereInfo& sphereInfo, const Vector3D& position,
         const double scale);

  void set_draw_styles(DrawStyle* defaultStyle, DrawStyle* hoveredStyle,
                       DrawStyle* selectedStyle);

  virtual void draw();

  BBox get_bbox();

  // All functions that are unused, because spheres can't be selected yet.
  double test_selection(const Vector2D& p, const Matrix4x4& worldTo3DH,
                        double minW) {
    return -1;
  }

  virtual void drag(double x, double y, double dx, double dy,
                    const Matrix4x4& modelViewProj) {}

  virtual Info getInfo();

  BSDF* get_bsdf();
  StaticScene::SceneObject* get_static_object();

  /**
   * Rather than drawing the object geometry for display, this method draws the
   * object with unique colors that can be used to determine which object was
   * selected or "picked" by the cursor.  The parameter pickID is the lowest
   * consecutive integer that has so far not been used by any other object as
   * a picking ID.  (Draw colors are then derived from these IDs.)  This data
   * will be used by Scene::update_selection to make the final determination
   * of which object (and possibly element within that object) was picked.
   */
  virtual void draw_pick(int& pickID, bool transformed = false);

  /** Assigns attributes of the selection based on the ID of the
   * object that was picked.  Can assume that pickID was one of
   * the IDs generated during this object's call to draw_pick().
   */
  virtual void setSelection(int pickID, Selection& selection);

 private:
  double r;
  Vector3D p;
  BSDF* bsdf;
  DrawStyle* style;
};

}  // namespace DynamicScene
}  // namespace CMU462

#endif  // CMU462_DYNAMICSCENE_SPHERE_H
