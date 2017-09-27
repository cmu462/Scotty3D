#ifndef CMU462_COLLADA_LIGHTINFO_H
#define CMU462_COLLADA_LIGHTINFO_H

#include "CMU462/spectrum.h"

#include "collada_info.h"

namespace CMU462 {
namespace Collada {

// For some reason enum classes aren't working for me here; not sure why.
namespace LightType {
enum T { NONE, AMBIENT, DIRECTIONAL, AREA, POINT, SPOT };
}

class LightInfo : public Instance {
 public:
  LightInfo();

  LightType::T light_type;  ///< type

  Spectrum spectrum;  ///< spectrum

  Vector3D position;   ///< position
  Vector3D direction;  ///< direction
  Vector3D up;         ///< up

  float falloff_deg;  ///< degree of fall off angle
  float falloff_exp;  ///< fall out exponent

  float constant_att;   ///< constant attenuation factor
  float linear_att;     ///< linear attenuation factor
  float quadratic_att;  ///< quadratic attenuation factor

};  // struct Light

std::ostream& operator<<(std::ostream& os, const LightInfo& light);

}  // namespace Collada
}  // namespace CMU462

#endif  // CMU462_COLLADA_LIGHTINFO_H
