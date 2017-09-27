#ifndef CMU462_COLLADA_MATERIALINFO_H
#define CMU462_COLLADA_MATERIALINFO_H

#include "CMU462/color.h"
#include "collada_info.h"
#include "../bsdf.h"

namespace CMU462 {
namespace Collada {

struct MaterialInfo : public Instance {
  BSDF* bsdf;

  // Texture* tex; ///< texture

};  // struct Material

std::ostream& operator<<(std::ostream& os, const MaterialInfo& material);

}  // namespace Collada
}  // namespace CMU462

#endif  // CMU462_COLLADA_MATERIALINFO_H
