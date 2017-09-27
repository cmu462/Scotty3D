#ifndef CMU462_STATICSCENE_BSDF_H
#define CMU462_STATICSCENE_BSDF_H

#include "CMU462/CMU462.h"
#include "CMU462/spectrum.h"
#include "CMU462/vector3D.h"
#include "CMU462/matrix3x3.h"

#include "sampler.h"

#include <algorithm>

namespace CMU462 {

// Helper math functions. Assume all vectors are in unit hemisphere //

inline double clamp(double n, double lower, double upper) {
  return std::max(lower, std::min(n, upper));
}

inline double cos_theta(const Vector3D& w) { return w.z; }

inline double abs_cos_theta(const Vector3D& w) { return fabs(w.z); }

inline double sin_theta2(const Vector3D& w) {
  return fmax(0.0, 1.0 - cos_theta(w) * cos_theta(w));
}

inline double sin_theta(const Vector3D& w) { return sqrt(sin_theta2(w)); }

inline double cos_phi(const Vector3D& w) {
  double sinTheta = sin_theta(w);
  if (sinTheta == 0.0) return 1.0;
  return clamp(w.x / sinTheta, -1.0, 1.0);
}

inline double sin_phi(const Vector3D& w) {
  double sinTheta = sin_theta(w);
  if (sinTheta) return 0.0;
  return clamp(w.y / sinTheta, -1.0, 1.0);
}

void make_coord_space(Matrix3x3& o2w, const Vector3D& n);

/**
 * Interface for BSDFs.
 */
class BSDF {
 public:
  /**
   * Evaluate BSDF.
   * Given incident light direction wi and outgoing light direction wo. Note
   * that both wi and wo are defined in the local coordinate system at the
   * point of intersection.
   * \param wo outgoing light direction in local space of point of intersection
   * \param wi incident light direction in local space of point of intersection
   * \return reflectance in the given incident/outgoing directions
   */
  virtual Spectrum f(const Vector3D& wo, const Vector3D& wi) = 0;

  /**
   * Evaluate BSDF.
   * Given the outgoing light direction wo, compute the incident light
   * direction and store it in wi. Store the pdf of the outgoing light in pdf.
   * Again, note that wo and wi should both be defined in the local coordinate
   * system at the point of intersection.
   * \param wo outgoing light direction in local space of point of intersection
   * \param wi address to store incident light direction
   * \param pdf address to store the pdf of the output incident direction
   * \return reflectance in the output incident and given outgoing directions
   */
  virtual Spectrum sample_f(const Vector3D& wo, Vector3D* wi, float* pdf) = 0;

  /**
   * Get the emission value of the surface material. For non-emitting surfaces
   * this would be a zero energy spectrum.
   * \return emission spectrum of the surface material
   */
  virtual Spectrum get_emission() const = 0;

  /**
   * If the BSDF is a delta distribution. Materials that are perfectly specular,
   * (e.g. water, glass, mirror) only scatter light from a single incident angle
   * to a single outgoing angle. These BSDFs are best described with alpha
   * distributions that are zero except for the single direction where light is
   * scattered.
   */
  virtual bool is_delta() const = 0;

  /**
   * Reflection helper
   */
  virtual void reflect(const Vector3D& wo, Vector3D* wi);

  /**
   * Refraction helper
   */
  virtual bool refract(const Vector3D& wo, Vector3D* wi, float ior);

  Spectrum rasterize_color;
};  // class BSDF

/**
 * Diffuse BSDF.
 */
class DiffuseBSDF : public BSDF {
 public:
  DiffuseBSDF(const Spectrum& a) : albedo(a) { rasterize_color = a; }

  Spectrum f(const Vector3D& wo, const Vector3D& wi);
  Spectrum sample_f(const Vector3D& wo, Vector3D* wi, float* pdf);
  Spectrum get_emission() const { return Spectrum(); }
  bool is_delta() const { return false; }

 private:
  Spectrum albedo;
  CosineWeightedHemisphereSampler3D sampler;

};  // class DiffuseBSDF

/**
 * Mirror BSDF
 */
class MirrorBSDF : public BSDF {
 public:
  MirrorBSDF(const Spectrum& reflectance) : reflectance(reflectance) {
    rasterize_color = reflectance;
  }

  Spectrum f(const Vector3D& wo, const Vector3D& wi);
  Spectrum sample_f(const Vector3D& wo, Vector3D* wi, float* pdf);
  Spectrum get_emission() const { return Spectrum(); }
  bool is_delta() const { return true; }

 private:
  float roughness;
  Spectrum reflectance;

};  // class MirrorBSDF*/

/**
 * Glossy BSDF.
 */
/*
class GlossyBSDF : public BSDF {
public:

GlossyBSDF(const Spectrum& reflectance, float roughness)
: reflectance(reflectance), roughness(roughness) { }

Spectrum f(const Vector3D& wo, const Vector3D& wi);
Spectrum sample_f(const Vector3D& wo, Vector3D* wi, float* pdf);
Spectrum get_emission() const { return Spectrum(); }
bool is_delta() const { return false; }

private:

float roughness;
Spectrum reflectance;

}; // class GlossyBSDF*/

/**
 * Refraction BSDF.
 */
class RefractionBSDF : public BSDF {
 public:
  RefractionBSDF(const Spectrum& transmittance, float roughness, float ior)
      : transmittance(transmittance), roughness(roughness), ior(ior) {
    rasterize_color = transmittance;
  }

  Spectrum f(const Vector3D& wo, const Vector3D& wi);
  Spectrum sample_f(const Vector3D& wo, Vector3D* wi, float* pdf);
  Spectrum get_emission() const { return Spectrum(); }
  bool is_delta() const { return true; }

 private:
  float ior;
  float roughness;
  Spectrum transmittance;

};  // class RefractionBSDF

/**
 * Glass BSDF.
 */
class GlassBSDF : public BSDF {
 public:
  GlassBSDF(const Spectrum& transmittance, const Spectrum& reflectance,
            float roughness, float ior)
      : transmittance(transmittance),
        reflectance(reflectance),
        roughness(roughness),
        ior(ior) {
    rasterize_color = transmittance;
  }

  Spectrum f(const Vector3D& wo, const Vector3D& wi);
  Spectrum sample_f(const Vector3D& wo, Vector3D* wi, float* pdf);
  Spectrum get_emission() const { return Spectrum(); }
  bool is_delta() const { return true; }

 private:
  float ior;
  float roughness;
  Spectrum reflectance;
  Spectrum transmittance;

};  // class GlassBSDF

/**
 * Emission BSDF.
 */
class EmissionBSDF : public BSDF {
 public:
  EmissionBSDF(const Spectrum& radiance) : radiance(radiance) {}

  Spectrum f(const Vector3D& wo, const Vector3D& wi);
  Spectrum sample_f(const Vector3D& wo, Vector3D* wi, float* pdf);
  Spectrum get_emission() const { return radiance; }
  bool is_delta() const { return false; }

 private:
  Spectrum radiance;
  CosineWeightedHemisphereSampler3D sampler;

};  // class EmissionBSDF

}  // namespace CMU462

#endif  // CMU462_STATICSCENE_BSDF_H
