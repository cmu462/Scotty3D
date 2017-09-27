#ifndef CMU462_COLLADA_SPHEREINFO_H
#define CMU462_COLLADA_SPHEREINFO_H

#include "collada_info.h"
#include "material_info.h"

namespace CMU462 {
namespace Collada {

struct SphereInfo : Instance {
  float radius;            ///< radius
  MaterialInfo* material;  ///< material of the sphere
};                         // struct Sphere

std::ostream& operator<<(std::ostream& os, const SphereInfo& sphere);

}  // namespace Collada
}  // namespace CMU462

#endif  // CMU462_COLLADA_SPHEREINFO_H
