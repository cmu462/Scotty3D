#ifndef CMU462_RAY_H
#define CMU462_RAY_H

#include "CMU462/CMU462.h"
#include "CMU462/vector3D.h"
#include "CMU462/vector4D.h"
#include "CMU462/matrix4x4.h"
#include "CMU462/spectrum.h"

namespace CMU462 {

struct Ray {
  size_t depth;  ///< depth of the Ray

  Vector3D o;            ///< origin
  Vector3D d;            ///< direction
  mutable double min_t;  ///< treat the ray as a segment (ray "begin" at max_t)
  mutable double max_t;  ///< treat the ray as a segment (ray "ends" at max_t)

  Vector3D inv_d;  ///< component wise inverse
  int sign[3];     ///< fast ray-bbox intersection

  /**
   * Constructor.
   * Create a ray instance with given origin and direction.
   * \param o origin of the ray
   * \param d direction of the ray
   * \param depth depth of the ray
   */
  Ray(const Vector3D& o, const Vector3D& d, int depth = 0)
      : o(o), d(d), min_t(0.0), max_t(INF_D), depth(depth) {
    inv_d = Vector3D(1 / d.x, 1 / d.y, 1 / d.z);
    sign[0] = (inv_d.x < 0);
    sign[1] = (inv_d.y < 0);
    sign[2] = (inv_d.z < 0);
  }

  /**
   * Constructor.
   * Create a ray instance with given origin and direction.
   * \param o origin of the ray
   * \param d direction of the ray
   * \param max_t max t value for the ray (if it's actually a segment)
   * \param depth depth of the ray
   */
  Ray(const Vector3D& o, const Vector3D& d, double max_t, int depth = 0)
      : o(o), d(d), min_t(0.0), max_t(max_t), depth(depth) {
    inv_d = Vector3D(1 / d.x, 1 / d.y, 1 / d.z);
    sign[0] = (inv_d.x < 0);
    sign[1] = (inv_d.y < 0);
    sign[2] = (inv_d.z < 0);
  }

  /**
   * Returns the point t * |d| along the ray.
   */
  inline Vector3D at_time(double t) const { return o + t * d; }

  /**
   * Returns the result of transforming the ray by the given transformation
   * matrix.
   */
  Ray transform_by(const Matrix4x4& t) const {
    const Vector4D& newO = t * Vector4D(o, 1.0);
    return Ray((newO / newO.w).to3D(), (t * Vector4D(d, 0.0)).to3D());
  }
};

// structure used for logging rays for subsequent visualization
struct LoggedRay {
  LoggedRay(const Ray& r, double hit_t) : o(r.o), d(r.d), hit_t(hit_t) {}

  Vector3D o;
  Vector3D d;
  double hit_t;
};

}  // namespace CMU462

#endif  // CMU462_RAY_H
