#include "camera_info.h"

namespace CMU462 {
namespace Collada {

std::ostream& operator<<(std::ostream& os, const CameraInfo& camera) {
  return os << "CameraInfo: " << camera.name << " (id: " << camera.id << ")"
            << " ["
            << " hfov=" << camera.hFov << " vfov=" << camera.vFov
            << " nclip=" << camera.nClip << " fclip=" << camera.fClip << " ]";
}

}  // namespace Collada
}  // namespace CMU462
