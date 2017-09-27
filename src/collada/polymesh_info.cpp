#include "polymesh_info.h"

using namespace std;

namespace CMU462 {
namespace Collada {

std::ostream& operator<<(std::ostream& os, const PolymeshInfo& polymesh) {
  os << "PolymeshInfo: " << polymesh.name << " (id:" << polymesh.id << ")";

  os << " [";

  os << " num_polygons=" << polymesh.polygons.size();
  os << " num_vertices=" << polymesh.vertices.size();
  os << " num_normals=" << polymesh.normals.size();
  os << " num_texcoords=" << polymesh.texcoords.size();

  os << " ]";

  return os;
}

}  // namespace Collada
}  // namespace CMU462
