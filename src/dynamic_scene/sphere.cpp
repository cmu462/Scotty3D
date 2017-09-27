#include "sphere.h"

#include "../static_scene/object.h"
#include "../misc/sphere_drawing.h"

namespace CMU462 {
namespace DynamicScene {

Sphere::Sphere(const Collada::SphereInfo& info, const Vector3D& position,
               const double scale)
    : p(position), r(info.radius * scale) {
  if (info.material) {
    bsdf = info.material->bsdf;
  } else {
    bsdf = new DiffuseBSDF(Spectrum(0.5f, 0.5f, 0.5f));
  }
}

void Sphere::set_draw_styles(DrawStyle* defaultStyle, DrawStyle* hoveredStyle,
                             DrawStyle* selectedStyle) {
  style = defaultStyle;
}

void Sphere::draw() {
  glTranslatef(position.x, position.y, position.z);
  Misc::draw_sphere_opengl(p, r);
}

void Sphere::draw_pick(int& pickID, bool transformed) {
  // TODO Currently, since nothing is drawn, spheres cannot be picked!
}

void Sphere::setSelection(int pickID, Selection& selection) {
  // Set the selection to this whole sphere
  selection.object = this;
  selection.element = NULL;
}

Info Sphere::getInfo() {
  Info info;
  info.push_back("SPHERE");
  return info;
}

BBox Sphere::get_bbox() {
  return BBox(p.x - r, p.y - r, p.z - r, p.x + r, p.y + r, p.z + r);
}

BSDF* Sphere::get_bsdf() { return bsdf; }

StaticScene::SceneObject* Sphere::get_static_object() {
  return new StaticScene::SphereObject(p, r, bsdf);
}

}  // namespace DynamicScene
}  // namespace CMU462
