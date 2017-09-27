#include "quaternion.h"

namespace CMU462 {

  std::ostream& operator<<( std::ostream& os, const Quaternion& v ) {
    os << "{ " << v.x << "i, " << v.y << "j, " << v.z << "k, " << v.w << " }";
    return os;
  }

} // namespace CMU462
