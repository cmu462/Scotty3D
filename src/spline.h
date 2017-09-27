#ifndef SPLINE_H
#define SPLINE_H

/*
 * Character class and Joint Class. Written by Bryce Summers on October 29,
 * 2015.
 *
 * Purpose : These classes represent a rigged character that can be animated.
 *
 */

#include <map>
#include <cmath>
using namespace std;

namespace CMU462 {
template <class T>
class Spline {
 public:
  Spline() {}
  ~Spline() {}

  // for each knot value (specified by a double), this map stores
  // the associated value (specified by an object of type T)
  map<double, T> knots;

  // convenience types
  typedef typename map<double, T>::iterator KnotIter;
  typedef typename map<double, T>::const_iterator KnotCIter;

  // Returns the interpolated value.  Optionally, one can request
  // a derivative of the spline (0 = no derivative, 1 = first derivative,
  // 2 = 2nd derivative).
  T evaluate(double time, int derivative = 0);

  // Purely for convenience, returns the exact same
  // value as Spline::evaluate()---simply lets one
  // evaluate a spline f as though it were a function f(t)
  // (which it is!)
  T operator()(double time);

  // Sets the value of the spline at a given time (i.e., knot),
  // creating a new knot at this time if necessary.
  void setValue(double time, T value);

  // Removes the knot closest to the given time, within the
  // given tolerance. Returns true iff a knot was removed.
  bool removeKnot(double time, double tolerance = .001);

 protected:
  // Given a time between 0 and 1, evaluates a cubic polynomial with
  // the given endpoint and tangent values at the beginning (0) and
  // end (1) of the interval.  Optionally, one can request a derivative
  // of the spline (0=no derivative, 1=first derivative, 2=2nd derivative).
  T cubicSplineUnitInterval(const T& position0, const T& position1,
                            const T& tangent0, const T& tangent1,
                            double normalizedTime, int derivative = 0);
};

#include "spline.inl"  // implementation
}

#endif  // SPLINE_H
