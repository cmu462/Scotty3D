#ifndef CMU462_SAMPLER_H
#define CMU462_SAMPLER_H

#include "CMU462/vector2D.h"
#include "CMU462/vector3D.h"
#include "CMU462/misc.h"

namespace CMU462 {

/**
 * Interface for generating point samples within the unit square
 */
class Sampler2D {
 public:
  /**
   * Virtual destructor.
   */
  virtual ~Sampler2D() {}

  /**
   * Take a point sample of the unit square
   */
  virtual Vector2D get_sample() const = 0;

};  // class Sampler2D

/**
 * Interface for generating 3D vector samples
 */
class Sampler3D {
 public:
  /**
   * Virtual destructor.
   */
  virtual ~Sampler3D() {}

  /**
   * Take a vector sample of the unit hemisphere
   */
  virtual Vector3D get_sample() const = 0;

};  // class Sampler3D

/**
 * A Sampler2D implementation with uniform distribution on unit square
 */
class UniformGridSampler2D : public Sampler2D {
 public:
  Vector2D get_sample() const;

};  // class UniformSampler2D

/**
 * A Sampler3D implementation with uniform distribution on unit hemisphere
 */
class UniformHemisphereSampler3D : public Sampler3D {
 public:
  Vector3D get_sample() const;

};  // class UniformHemisphereSampler3D

/**
 * A Sampler3D implementation with cosine-weighted distribution on unit
 * hemisphere.
 */
class CosineWeightedHemisphereSampler3D : public Sampler3D {
 public:
  Vector3D get_sample() const;
  // Also returns the pdf at the sample point for use in importance sampling.
  Vector3D get_sample(float* pdf) const;

};  // class UniformHemisphereSampler3D

/**
 * TODO (extra credit) :
 * Jittered sampler implementations
 */

}  // namespace CMU462

#endif  // CMU462_SAMPLER_H
