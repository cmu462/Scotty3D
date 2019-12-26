#include "bsdf.h"

#include <algorithm>
#include <iostream>
#include <utility>


using std::min;
using std::max;
using std::swap;

namespace CMU462 {

void make_coord_space(Matrix3x3& o2w, const Vector3D& n) {
  Vector3D z = Vector3D(n.x, n.y, n.z);
  Vector3D h = z;
  if (fabs(h.x) <= fabs(h.y) && fabs(h.x) <= fabs(h.z))
    h.x = 1.0;
  else if (fabs(h.y) <= fabs(h.x) && fabs(h.y) <= fabs(h.z))
    h.y = 1.0;
  else
    h.z = 1.0;

  z.normalize();
  Vector3D y = cross(h, z);
  y.normalize();
  Vector3D x = cross(z, y);
  x.normalize();

  o2w[0] = x;
  o2w[1] = y;
  o2w[2] = z;
}

// Diffuse BSDF //

Spectrum DiffuseBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  return albedo * (1.0 / PI);
}

Spectrum DiffuseBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
	// TODO (PathTracer):
	// Implement DiffuseBSDF

	// use Cosine-Weighted sampling
	*wi = sampler.get_sample();
	*pdf = wi->z / PI; // wi->z = cos(theta)

	return f(wo,*wi);
}

// Mirror BSDF //

Spectrum MirrorBSDF::f(const Vector3D& wo, const Vector3D& wi) {
	Vector3D N(0, 0, 1);
	Vector3D H = (wo + wi).unit();
	return reflectance * pow(std::max(dot(N, H), 0.0), roughness);
	//return Spectrum(1.0f, 1.0f, 1.0f);
}

Spectrum MirrorBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
	// TODO (PathTracer):
	// Implement MirrorBSDF

	reflect(wo, wi);
	*pdf = 1.0f;
	return reflectance * (1.0 / abs_cos_theta(*wi));
	/*reflect(wo, wi);
	*pdf = 1.0;
	return f(wo, *wi);*/
}

// Glossy BSDF //

/*
Spectrum GlossyBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  return Spectrum();
}

Spectrum GlossyBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
  *pdf = 1.0f;
  return reflect(wo, wi, reflectance);
}
*/

// Refraction BSDF //

Spectrum RefractionBSDF::f(const Vector3D& wo, const Vector3D& wi) {
	//return Spectrum(1.0f, 1.0f, 1.0f);
	return Spectrum();
}

Spectrum RefractionBSDF::sample_f(const Vector3D& wo, Vector3D* wi,
	float* pdf) {
	// TODO (PathTracer):
	// Implement RefractionBSDF

	/*bool external = refract(wo, wi, ior);
	*pdf = 1.0;
	if (external) return f(wo, *wi);
	else return Spectrum();*/
	return Spectrum();
}

// Glass BSDF //

Spectrum GlassBSDF::f(const Vector3D& wo, const Vector3D& wi) {
	// this function is useless
	return Spectrum();
}

Spectrum GlassBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
	// TODO (PathTracer):
	// Compute Fresnel coefficient.
	float ni, nt, cos_theta_i, cos_theta_t;
	if (wo.z >= 0) {
		ni = 1.0;
		nt = ior;
	}
	else {
		ni = ior;
		nt = 1.0;
	}

	Vector3D wi_refract;
	bool external = refract(wo, &wi_refract, ior);
	if (external) cos_theta_t = abs(wi_refract.z);
	else cos_theta_t = 0.0f;
	cos_theta_i = abs(wo.z);

	float r_parallel = (nt* cos_theta_i - ni * cos_theta_t) / (nt * cos_theta_i + ni * cos_theta_t);
	float r_perpendicular = (ni* cos_theta_i - nt * cos_theta_t) / (ni * cos_theta_i + nt * cos_theta_t);
	float Fr = 0.5f*(r_parallel*r_parallel + r_perpendicular * r_perpendicular);
	Fr = std::min(1.0f, std::max(0.0f, Fr));
	Fr = 0.0;

	*pdf = 1.0;
	// decide either reflect or refract based on it
	if ((double(rand()) / RAND_MAX) < Fr) {
		reflect(wo, wi);
		//*pdf = Fr;
		return Spectrum(1.0f, 1.0f, 1.0f);
	}
	else {
		*wi = wi_refract;
		//*pdf = 1.0f - Fr;
		if (external) {
			// use Distribution Function for Transmitted Light
			//float x = nt * nt / ni / ni * (1.0f - Fr) / abs(cos_theta_i);
			float x = nt * nt / ni / ni * (1.0f - Fr);
			//printf("%f\n", x);
			return Spectrum(x, x, x)*transmittance;
			//return Spectrum(1.0f, 1.0f, 1.0f);
		}
		else return Spectrum();
	}
}

void BSDF::reflect(const Vector3D& wo, Vector3D* wi) {
	// TODO (PathTracer):
	// Implement reflection of wo about normal (0,0,1) and store result in wi.
	wi->z = wo.z;
	wi->x = -wo.x;
	wi->y = -wo.y;
}

bool BSDF::refract(const Vector3D& wo, Vector3D* wi, float ior) {
	// TODO (PathTracer):
	// Use Snell's Law to refract wo surface and store result ray in wi.
	// Return false if refraction does not occur due to total internal reflection
	// and true otherwise. When dot(wo,n) is positive, then wo corresponds to a
	// ray entering the surface through vacuum.
	float theta2, sin_theta2;
	float theta1 = acosf(abs(wo.z));
	if (wo.z >= 0) sin_theta2 = 1.0f * sin(theta1) / ior;
	else sin_theta2 = ior * sin(theta1) / 1.0f;
	if (sin_theta2 > 1.0f) {
		wi->x = -wo.x;
		wi->y = -wo.y;
		wi->z = 0;
		wi->normalize();
		return false;
	}
	else theta2 = asinf(sin_theta2);

	wi->x = -wo.x;
	wi->y = -wo.y;
	wi->z = sqrt(wi->x*wi->x + wi->y*wi->y) / tan(theta2);
	wi->normalize();

	if (wo.z >= 0) wi->z *= -1.0;
	return true;
}

// Emission BSDF //

Spectrum EmissionBSDF::f(const Vector3D& wo, const Vector3D& wi) {
  return Spectrum();
}

Spectrum EmissionBSDF::sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) {
  *wi = sampler.get_sample(pdf);
  return Spectrum();
}

}  // namespace CMU462
