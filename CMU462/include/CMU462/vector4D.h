#ifndef CMU462_VECTOR4D_H
#define CMU462_VECTOR4D_H

#include "CMU462.h"
#include "vector3D.h"

#include <ostream>
#include <cmath>

namespace CMU462 {

/**
 * Defines 4D standard vectors.
 */
class Vector4D {
 public:

  /**
   * this type-punning is still technically undefined behavior, but all major
   * compilers explicitly define it (gcc, clang, msvc)
  */
  union {
    struct { double x; double y; double z; double w; };
    std::array<double, 4> data;
  };

  /**
   * Constructor.
   * Initializes tp vector (0,0,0, 0).
   */
 Vector4D() : x( 0.0 ), y( 0.0 ), z( 0.0 ), w( 0.0 ) { }

  /**
   * Constructor.
   * Initializes to vector (x,y,z,w).
   */
 Vector4D( double x, double y, double z, double w) : x( x ), y( y ), z( z ), w( w ) { }

  /**
   * Constructor.
   * Initializes to vector (x,y,z,0).
   */
 Vector4D( double x, double y, double z) : x( x ), y( y ), z( z ), w( 0.0 ) { }


  /**
   * Constructor.
   * Initializes to vector (c,c,c,c)
   */
 Vector4D( double c ) : x( c ), y( c ), z( c ), w ( c ) { }

  /**
   * Constructor.
   * Initializes from existing vector4D.
   */
 Vector4D( const Vector4D& v ) : x( v.x ), y( v.y ), z( v.z ), w( v.w ) { }

  /**
   * Constructor.
   * Initializes from existing vector3D.
   */
 Vector4D( const Vector3D& v ) : x( v.x ), y( v.y ), z( v.z ), w( 0.0 ) { }

 /**
   * Constructor.
   * Initializes from existing vector3D and w value.
   */
 Vector4D( const Vector3D& v, double w ) : x( v.x ), y( v.y ), z( v.z ), w( w ) { }

  // returns reference to the specified component (0-based indexing: x, y, z)
  inline double& operator[] ( const int& index ) {
    return data[ index ];
  }

  // returns const reference to the specified component (0-based indexing: x, y, z)
  inline const double& operator[] ( const int& index ) const {
    return data[ index ];
  }

  // negation
  inline Vector4D operator-( void ) const {
    return Vector4D( -x, -y, -z, -w);
  }

  // addition
  inline Vector4D operator+( const Vector4D& v ) const {
    return Vector4D( x + v.x, y + v.y, z + v.z, w + v.w);
  }

  // subtraction
  inline Vector4D operator-( const Vector4D& v ) const {
    return Vector4D( x - v.x, y - v.y, z - v.z, w - v.w );
  }

  // right scalar multiplication
  inline Vector4D operator*( const double& c ) const {
    return Vector4D( x * c, y * c, z * c, w * c );
  }

  // scalar division
  inline Vector4D operator/( const double& c ) const {
    const double rc = 1.0/c;
    return Vector4D( rc * x, rc * y, rc * z, rc * w );
  }

  // addition / assignment
  inline void operator+=( const Vector4D& v ) {
    x += v.x; y += v.y; z += v.z; z += v.w;
  }

  // subtraction / assignment
  inline void operator-=( const Vector4D& v ) {
    x -= v.x; y -= v.y; z -= v.z; w -= v.w;
  }

  // scalar multiplication / assignment
  inline void operator*=( const double& c ) {
    x *= c; y *= c; z *= c; w *= c;
  }

  // scalar division / assignment
  inline void operator/=( const double& c ) {
    (*this) *= ( 1./c );
  }

  /**
   * Returns Euclidean distance metric extended to 4 dimensions.
   */
  inline double norm( void ) const {
    return sqrt( x*x + y*y + z*z + w*w );
  }

  /**
   * Returns Euclidean length squared.
   */
  inline double norm2( void ) const {
    return x*x + y*y + z*z + w*w;
  }

  /**
   * Returns unit vector. (returns the normalized copy of this vector.)
   */
  inline Vector4D unit( void ) const {
    double rNorm = 1. / sqrt( x*x + y*y + z*z + w*w);
    return Vector4D( rNorm*x, rNorm*y, rNorm*z );
  }

  /**
   * Divides by Euclidean length.
   * This vector will be of unit length i.e. "normalized" afterwards.
   */
  inline void normalize( void ) {
    (*this) /= norm();
  }

  /**
   * Converts this vector to a 3D vector ignoring the w component.
   */
  Vector3D to3D();

  /**
   * Converts this vector to a 3D vector by dividing x, y, and z by w.
   */
  Vector3D projectTo3D();

}; // class Vector4D

// left scalar multiplication
inline Vector4D operator* ( const double& c, const Vector4D& v ) {
  return Vector4D( c * v.x, c * v.y, c * v.z, c*v.w );
}

// dot product (a.k.a. inner or scalar product)
inline double dot( const Vector4D& u, const Vector4D& v ) {
  return u.x*v.x + u.y*v.y + u.z*v.z + u.w*v.w;;
}

// prints components
std::ostream& operator<<( std::ostream& os, const Vector4D& v );

} // namespace CMU462

#endif // CMU462_VECTOR3D_H
