#include "light_info.h"

using namespace std;

namespace CMU462 {
namespace Collada {

LightInfo::LightInfo() {
  light_type = LightType::NONE;

  spectrum = Spectrum(1, 1, 1);

  position = Vector3D(0, 0, 0);    // COLLADA default
  direction = Vector3D(0, 0, -1);  // COLLADA default
  up = Vector3D(0, 1, 0);          // CMU462 default

  float falloff_deg = 45;
  float falloff_exp = 0.15;

  constant_att = 1;
  linear_att = 0;
  quadratic_att = 0;
}

std::ostream& operator<<(std::ostream& os, const LightInfo& light) {
  os << "LightInfo: " << light.name << " (id:" << light.id << ")";

  os << " [";

  switch (light.light_type) {
    case LightType::NONE:
      os << "type=none";
    case LightType::AMBIENT:
      os << " type=ambient"
         << " spectrum=" << light.spectrum;
      break;
    case LightType::DIRECTIONAL:
      os << " type=directional"
         << " spectrum=" << light.spectrum << " direction=" << light.direction;
      break;
    case LightType::AREA:
      os << " type=area"
         << " spectrum=" << light.spectrum << " direction=" << light.direction;
      break;
    case LightType::POINT:
      os << " type=point"
         << " spectrum=" << light.spectrum << " position=" << light.position
         << " constant_att=" << light.constant_att
         << " linear_att=" << light.linear_att
         << " quadratic_att=" << light.quadratic_att;
      break;
    case LightType::SPOT:
      os << " type=spot"
         << " spectrum=" << light.spectrum << " position=" << light.position
         << " direction=" << light.direction
         << " falloff_deg=" << light.falloff_deg
         << " falloff_exp=" << light.falloff_exp
         << " constant_att=" << light.constant_att
         << " linear_att=" << light.linear_att
         << " quadratic_att=" << light.quadratic_att;
      break;
  }

  os << " ]";

  return os;
}

}  // namespace Collada
}  // namespace CMU462
