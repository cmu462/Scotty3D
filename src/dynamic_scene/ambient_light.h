#ifndef CMU462_DYNAMICSCENE_AMBIENTLIGHT_H
#define CMU462_DYNAMICSCENE_AMBIENTLIGHT_H

#include "scene.h"
#include "../static_scene/light.h"

namespace CMU462 {
namespace DynamicScene {

class AmbientLight : public SceneLight {
 public:
  AmbientLight(const Collada::LightInfo& light_info) {
    this->spectrum = light_info.spectrum;
  }

  StaticScene::SceneLight* get_static_light() const {
    StaticScene::InfiniteHemisphereLight* l =
        new StaticScene::InfiniteHemisphereLight(spectrum);
    return l;
  }

 private:
  Spectrum spectrum;
};

}  // namespace DynamicScene
}  // namespace CMU462

#endif  // CMU462_DYNAMICSCENE_AMBIENTLIGHT_H
