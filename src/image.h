#ifndef CMU462_IMAGE_H
#define CMU462_IMAGE_H

#include "CMU462/color.h"
#include "CMU462/spectrum.h"

#include <vector>
#include <string.h>

namespace CMU462 {

/**
 * Image buffer which stores color space values with RGBA pixel layout using
 * 32 bit unsigned integers (8-bits per color channel).
 */
struct ImageBuffer {
  /**
   * Default constructor.
   * The default constructor creates a zero-sized image.
   */
  ImageBuffer() : w(0), h(0) {}

  /**
   * Parameterized constructor.
   * Create an image of given size.
   * \param w width of the image
   * \param h height of the image
   */
  ImageBuffer(size_t w, size_t h) : w(w), h(h) { data.resize(w * h); }

  /**
   * Resize the image buffer.
   * \param w new width of the image
   * \param h new height of the image
   */
  void resize(size_t w, size_t h) {
    this->w = w;
    this->h = h;
    data.resize(w * h);
    clear();
  }

  /**
   * Update the color of a given pixel.
   * \param c color value to be set
   * \param x row of the pixel
   * \param y column of the pixel
   */
  void update_pixel(const Color& c, size_t x, size_t y) {
    // assert(0 <= x && x < w);
    // assert(0 <= y && y < h);
    uint32_t p = 0;
    p += ((uint32_t)(clamp(0.f, 1.f, c.a) * 255)) << 24;
    p += ((uint32_t)(clamp(0.f, 1.f, c.b) * 255)) << 16;
    p += ((uint32_t)(clamp(0.f, 1.f, c.g) * 255)) << 8;
    p += ((uint32_t)(clamp(0.f, 1.f, c.r) * 255));
    data[x + y * w] = p;
  }

  /**
   * If the buffer is empty.
   */
  bool is_empty() { return (w == 0 && h == 0); }

  /**
   * Clear image data.
   */
  void clear() {
    if (data.size() > 0) memset(&data[0], 0, w * h * sizeof(uint32_t));
  }

  size_t w;                    ///< width
  size_t h;                    ///< height
  std::vector<uint32_t> data;  ///< pixel buffer
};

/**
 * High Dynamic Range image buffer which stores linear space spectrum
 * values with 32 bit floating points.
 */
struct HDRImageBuffer {
  /**
   * Default constructor.
   * The default constructor creates a zero-sized image.
   */
  HDRImageBuffer() : w(0), h(0) {}

  /**
   * Parameterized constructor.
   * Create an image of given size.
   * \param w width of the image
   * \param h height of the image
   */
  HDRImageBuffer(size_t w, size_t h) : w(w), h(h) { data.resize(w * h); }

  /**
   * Resize the image buffer.
   * \param w new width of the image
   * \param h new height of the image
   */
  void resize(size_t w, size_t h) {
    this->w = w;
    this->h = h;
    data.resize(w * h);
    clear();
  }

  /**
   * Update the color of a given pixel.
   * \param s new spectrum value to be set
   * \param x row of the pixel
   * \param y column of the pixel
   */
  void update_pixel(const Spectrum& s, size_t x, size_t y) {
    // assert(0 <= x && x < w);
    // assert(0 <= y && y < h);
    data[x + y * w] = s;
  }

  /**
   * Update the color of a given pixel. Blend new pixel color with current
   * pixel data in the buffer according to the blending factor. The result
   * of this would be buffer[i] = s * r + buffer[i] * (1-r);
   * \param s new spectrum value to be set
   * \param x row of the pixel
   * \param y column of the pixel
   * \param r blending factor
   */
  void update_pixel(const Spectrum& s, size_t x, size_t y, float r) {
    // assert(0 <= x && x < w);
    // assert(0 <= y && y < h);
    data[x + y * w] = s * r + (1 - r) * data[x + y * w];
  }

  /**
   * Tonemap and convert to color space image.
   * \param target target color buffer to store output
   * \param gamma gamma value
   * \param level exposure level adjustment
   * \key   key value to map average tone to (higher means brighter)
   * \why   white point (higher means larger dynamic range)
   */
  void tonemap(ImageBuffer& target, float gamma, float level, float key,
               float wht) {
    // compute global log average luminance!
    float avg = 0;
    for (size_t i = 0; i < w * h; ++i) {
      // the small delta value below is used to avoids singularity
      avg += log(0.0000001f + data[i].illum());
    }
    avg = exp(avg / (w * h));

    // apply on pixels
    float one_over_gamma = 1.0f / gamma;
    float exposure = sqrt(pow(2, level));
    for (size_t y = 0; y < h; ++y) {
      for (size_t x = 0; x < w; ++x) {
        Spectrum s = data[x + y * w];
        float l = s.illum();
        s *= key / avg;
        s *= ((l + 1) / (wht * wht)) / (l + 1);
        float r = pow(s.r * exposure, one_over_gamma);
        float g = pow(s.g * exposure, one_over_gamma);
        float b = pow(s.b * exposure, one_over_gamma);
        target.update_pixel(Color(r, g, b, 1.0), x, y);
      }
    }
  }

  /**
   * Convert the given tile of the buffer to color.
   */
  void toColor(ImageBuffer& target, size_t x0, size_t y0, size_t x1,
               size_t y1) {
    float gamma = 2.2f;
    float level = 1.0f;
    float one_over_gamma = 1.0f / gamma;
    float exposure = sqrt(pow(2, level));
    for (size_t y = y0; y < y1; ++y) {
      for (size_t x = x0; x < x1; ++x) {
        const Spectrum& s = data[x + y * w];
        float r = pow(s.r * exposure, one_over_gamma);
        float g = pow(s.g * exposure, one_over_gamma);
        float b = pow(s.b * exposure, one_over_gamma);
        target.update_pixel(Color(r, g, b, 1.0), x, y);
      }
    }
  }

  /**
   * If the buffer is empty
   */
  bool is_empty() { return (w == 0 && h == 0); }

  /**
   * Clear image buffer.
   */
  void clear() {
    if (data.size() > 0) memset(&data[0], 0, w * h * sizeof(Spectrum));
  }

  size_t w;                    ///< width
  size_t h;                    ///< height
  std::vector<Spectrum> data;  ///< pixel buffer

};  // class HDRImageBuffer

}  // namespace CMU462

#endif  // CMU462_IMAGE_H
