#include "environment_light.h"

namespace CMU462 {
namespace StaticScene {

EnvironmentLight::EnvironmentLight(const HDRImageBuffer* envMap)
    : envMap(envMap) {
  // TODO: (PathTracer) initialize things here as needed
}

Spectrum EnvironmentLight::sample_L(const Vector3D& p, Vector3D* wi,
                                    float* distToLight, float* pdf) const {
  // TODO: (PathTracer) Implement
  return Spectrum(0, 0, 0);
}

Spectrum EnvironmentLight::sample_dir(const Ray& r) const {
  // TODO: (PathTracer) Implement
  return Spectrum(0, 0, 0);
}

}  // namespace StaticScene
}  // namespace CMU462
