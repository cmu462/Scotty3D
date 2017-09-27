#ifndef CMU462_DYNAMICSCENE_ENVIRONMENTLIGHT_H
#define CMU462_DYNAMICSCENE_ENVIRONMENTLIGHT_H

#include "scene.h"
#include "../image.h"
#include "../static_scene/light.h"

namespace CMU462 {
namespace DynamicScene {

class EnvironmentLight : public SceneLight {
 public:
  EnvironmentLight(HDRImageBuffer* envmap) : envmap(envmap) {}

  StaticScene::SceneLight* get_static_light() const {
    StaticScene::EnvironmentLight* l =
        new StaticScene::EnvironmentLight(envmap);
    return l;
  }

 private:
  HDRImageBuffer* envmap;
};

}  // namespace DynamicScene
}  // namespace CMU462

#endif  // CMU462_DYNAMICSCENE_ENVIRONMENTLIGHT_H
