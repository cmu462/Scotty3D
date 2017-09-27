#ifndef CMU462_DYNAMICSCENE_AREALIGHT_H
#define CMU462_DYNAMICSCENE_AREALIGHT_H

#include "scene.h"
#include "../static_scene/light.h"

namespace CMU462 {
namespace DynamicScene {

class AreaLight : public SceneLight {
 public:
  AreaLight(const Collada::LightInfo& light_info, const Matrix4x4& transform) {
    this->spectrum = light_info.spectrum;
    this->position = (transform * Vector4D(light_info.position, 1)).to3D();
    this->direction =
        (transform * Vector4D(light_info.direction, 1)).to3D() - position;
    this->direction.normalize();

    Vector3D dim_y = light_info.up;
    Vector3D dim_x = cross(light_info.up, light_info.direction);

    this->dim_x = (transform * Vector4D(dim_x, 1)).to3D() - position;
    this->dim_y = (transform * Vector4D(dim_y, 1)).to3D() - position;
  }

  StaticScene::SceneLight* get_static_light() const {
    StaticScene::AreaLight* l =
        new StaticScene::AreaLight(spectrum, position, direction, dim_x, dim_y);
    return l;
  }

 private:
  Spectrum spectrum;
  Vector3D position;
  Vector3D direction;

  Vector3D dim_x;
  Vector3D dim_y;
};

}  // namespace DynamicScene
}  // namespace CMU462

#endif  // CMU462_DYNAMICSCENE_AREALIGHT_H
