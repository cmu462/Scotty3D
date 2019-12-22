#include "sampler.h"

namespace CMU462 {

// Uniform Sampler2D Implementation //

	Vector2D UniformGridSampler2D::get_sample() const {
		// TODO (PathTracer):
		// Implement uniform 2D grid sampler

		return Vector2D(double(rand()) / RAND_MAX, double(rand()) / RAND_MAX);
	}

// Uniform Hemisphere Sampler3D Implementation //

Vector3D UniformHemisphereSampler3D::get_sample() const {
  double Xi1 = (double)(std::rand()) / RAND_MAX;
  double Xi2 = (double)(std::rand()) / RAND_MAX;

  double theta = acos(Xi1);
  double phi = 2.0 * PI * Xi2;

  double xs = sinf(theta) * cosf(phi);
  double ys = sinf(theta) * sinf(phi);
  double zs = cosf(theta);

  return Vector3D(xs, ys, zs);
}

Vector3D CosineWeightedHemisphereSampler3D::get_sample() const {
  float f;
  return get_sample(&f);
}

Vector3D CosineWeightedHemisphereSampler3D::get_sample(float *pdf) const {
	// You may implement this, but don't have to.

	// uniform sample points in a unit circle
	// see slide 19 page 35
	double x1 = (double)(std::rand()) / RAND_MAX;
	double x2 = (double)(std::rand()) / RAND_MAX;
	double theta = 2.0*PI*x1;
	double r = sqrt(x2);
	double x = r * cos(theta);
	double y = r * sin(theta);

	// project them up onto the unit hemisphere
	return Vector3D(x,y,sqrt(1-x*x-y*y));
}

}  // namespace CMU462
