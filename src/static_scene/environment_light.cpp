#include "environment_light.h"

namespace CMU462 {
namespace StaticScene {

EnvironmentLight::EnvironmentLight(const HDRImageBuffer* envMap)
    : envMap(envMap) {
  // TODO: (PathTracer) initialize things here as needed
}

Spectrum EnvironmentLight::sample_L(const Vector3D& p, Vector3D* wi,
	float* distToLight, float* pdf) const {
	// TODO: (PathTracer) Implement

	// uniform sampling in the enitre sphere
	// by first sampling in hemisphere
	// then decide whether to flip the z coodinate
	double Xi1 = (double)(std::rand()) / RAND_MAX;
	double Xi2 = (double)(std::rand()) / RAND_MAX;

	double theta = acos(Xi1);
	double phi = 2.0 * PI * Xi2;

	double xs = sinf(theta) * cosf(phi);
	double ys = sinf(theta) * sinf(phi);
	double zs = cosf(theta);

	if ((double)(std::rand()) / RAND_MAX < 0.5) zs *= -1;
	Vector3D d = (xs, ys, zs);
	Ray r(p,d);

	*pdf = 1.0 / 4 / PI;
	*distToLight = INF_D;
	return sample_dir(r);
}

Spectrum EnvironmentLight::sample_dir(const Ray& r) const {
	// TODO: (PathTracer) Implement
	// in meshedit mode the green (y) axis is oriented vertically, not z
	double theta = acos(r.d.y); 
	double phi = atan2(r.d.x, r.d.z);
	if (phi < 0) phi += 2 * PI;
	double x = phi / (2 * PI) * envMap->w;
	double y = theta / PI * envMap->h;
	if (x >= 1.0) x--;
	if (y >= 1.0) y--;

	int x0 = int(x);
	int y0 = int(y);
	int x1 = (x0 + 1) % envMap->w;
	int y1 = y0 + 1;
	if (y0 == envMap->h - 1) y1 = y0;
	
	// implement bilinear interpolation
	Spectrum topleft = envMap->data[y0*envMap->w + x0];
	Spectrum topright = envMap->data[y0*envMap->w + x1];
	Spectrum bottomleft = envMap->data[y1*envMap->w + x0];
	Spectrum bottomright = envMap->data[y1*envMap->w + x1];

	Spectrum top = topleft + (topright + topleft * -1) * ((x - x0) / (x1 - x0));
	Spectrum bottom = bottomleft + (bottomright + bottomleft * -1) * ((x - x0) / (x1 - x0));
	Spectrum result = top + (bottom + top * -1) * ((y - y0) / (y1 - y0));
	return result;
	//return envMap->data[y0*envMap->w + x0];
}

}  // namespace StaticScene
}  // namespace CMU462
