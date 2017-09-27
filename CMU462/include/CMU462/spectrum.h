#ifndef CMU462_SPECTRUM_H
#define CMU462_SPECTRUM_H

#include "CMU462.h"
#include "color.h"

namespace CMU462 {

/**
 * Encodes radiance & irradiance values by the intensity of each visible
 * spectrum. Note that this is not strictly an actual spectrum with all
 * wavelengths, but it gives us enough information as we can only sense
 * a particular wavelengths.
 */
class Spectrum {
 public:
  float r;  ///< intensity of red spectrum
  float g;  ///< intensity of green spectrum
  float b;  ///< intensity of blue spectrum

  /**
   * Parameterized Constructor.
   * Initialize from component values.
   * \param r Intensity of the red spectrum
   * \param g Intensity of the green spectrum
   * \param b Intensity of the blue spectrum
   */
  Spectrum(float r = 0, float g = 0, float b = 0) : r(r), g(g), b(b) {}

  /**
   * Constructor.
   * Initialize from an 8-bit RGB color represented as a uint8_t array.
   * \param arr Array containing component values.
   */
  Spectrum(const uint8_t *arr);

  // operators //

  inline Spectrum operator+(const Spectrum &rhs) const {
    return Spectrum(r + rhs.r, g + rhs.g, b + rhs.b);
  }

  inline Spectrum &operator+=(const Spectrum &rhs) {
    r += rhs.r;
    g += rhs.g;
    b += rhs.b;
    return *this;
  }

  inline Spectrum operator*(const Spectrum &rhs) const {
    return Spectrum(r * rhs.r, g * rhs.g, b * rhs.b);
  }

  inline Spectrum &operator*=(const Spectrum &rhs) {
    r *= rhs.r;
    g *= rhs.g;
    b *= rhs.b;
    return *this;
  }

  inline Spectrum operator*(float s) const {
    return Spectrum(r * s, g * s, b * s);
  }

  inline Spectrum &operator*=(float s) {
    r *= s;
    g *= s;
    b *= s;
    return *this;
  }

  inline bool operator==(const Spectrum &rhs) const {
    return r == rhs.r && g == rhs.g && b == rhs.b;
  }

  inline bool operator!=(const Spectrum &rhs) const {
    return !operator==(rhs);
  }

  inline Color toColor() const {
    return Color(r, g, b, 1); 
  }

  inline float illum() const { 
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
  }

  static Spectrum fromColor(const Color &c) {
    return Spectrum(c.a * c.r, c.a * c.g, c.a * c.b);
  }


};  // class Spectrum

// Commutable scalar multiplication
inline Spectrum operator*(float s, const Spectrum &c) { return c * s; }

// Prints components
std::ostream &operator<<(std::ostream &os, const Spectrum &c);

}  // namespace CMU462

#endif  // CMU462_SPECTRUM_H
