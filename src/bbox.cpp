#include "bbox.h"

#include "GL/glew.h"

#include <algorithm>
#include <iostream>

namespace CMU462 {

	bool BBox::intersect(const Ray &r, double &t0, double &t1) const {
		// TODO (PathTracer):
		// Implement ray - bounding box intersection test
		// If the ray intersected the bounding box within the range given by
		// t0, t1, update t0 and t1 with the new intersection times.
		double ox = r.o.x;
		double oy = r.o.y;
		double oz = r.o.z;
		double dx = r.d.x;
		double dy = r.d.y;
		double dz = r.d.z;
		double tx_min = -INF_D, ty_min = -INF_D, tz_min = -INF_D;
		double tx_max = INF_D, ty_max = INF_D, tz_max = INF_D;
		double x0 = this->min.x;
		double y0 = this->min.y;
		double z0 = this->min.z;
		double x1 = this->max.x;
		double y1 = this->max.y;
		double z1 = this->max.z;

		if (abs(dx) < 0.000001) {
			if (ox < x0 || ox > x1) return false;
		}
		else {
			if (dx >= 0.0) {
				tx_min = (x0 - ox) / dx;
				tx_max = (x1 - ox) / dx;
			}
			else {
				tx_max = (x0 - ox) / dx;
				tx_min = (x1 - ox) / dx;
			}
		}

		if (abs(dy) < 0.000001) {
			if (oy < y0 || oy > y1) return false;
		}
		else {
			if (dy >= 0.0) {
				ty_min = (y0 - oy) / dy;
				ty_max = (y1 - oy) / dy;
			}
			else {
				ty_max = (y0 - oy) / dy;
				ty_min = (y1 - oy) / dy;
			}
		}

		if (abs(dz) < 0.000001) {
			if (oz < z0 || oz > z1) return false;
		}
		else {
			if (dz >= 0.0) {
				tz_min = (z0 - oz) / dz;
				tz_max = (z1 - oz) / dz;
			}
			else {
				tz_max = (z0 - oz) / dz;
				tz_min = (z1 - oz) / dz;
			}
		}

		t0 = std::max(tx_min, std::max(ty_min, tz_min));
		t1 = std::min(tx_max, std::min(ty_max, tz_max));

		return t0 < t1;
	}

void BBox::draw(Color c) const {
  glColor4f(c.r, c.g, c.b, c.a);

  // top
  glBegin(GL_LINE_STRIP);
  glVertex3d(max.x, max.y, max.z);
  glVertex3d(max.x, max.y, min.z);
  glVertex3d(min.x, max.y, min.z);
  glVertex3d(min.x, max.y, max.z);
  glVertex3d(max.x, max.y, max.z);
  glEnd();

  // bottom
  glBegin(GL_LINE_STRIP);
  glVertex3d(min.x, min.y, min.z);
  glVertex3d(min.x, min.y, max.z);
  glVertex3d(max.x, min.y, max.z);
  glVertex3d(max.x, min.y, min.z);
  glVertex3d(min.x, min.y, min.z);
  glEnd();

  // side
  glBegin(GL_LINES);
  glVertex3d(max.x, max.y, max.z);
  glVertex3d(max.x, min.y, max.z);
  glVertex3d(max.x, max.y, min.z);
  glVertex3d(max.x, min.y, min.z);
  glVertex3d(min.x, max.y, min.z);
  glVertex3d(min.x, min.y, min.z);
  glVertex3d(min.x, max.y, max.z);
  glVertex3d(min.x, min.y, max.z);
  glEnd();
}

std::ostream &operator<<(std::ostream &os, const BBox &b) {
  return os << "BBOX(" << b.min << ", " << b.max << ")";
}

}  // namespace CMU462
