#include "triangle.h"

#include "CMU462/CMU462.h"
#include "GL/glew.h"

namespace CMU462 {
namespace StaticScene {

Triangle::Triangle(const Mesh* mesh, vector<size_t>& v) : mesh(mesh), v(v) {}
Triangle::Triangle(const Mesh* mesh, size_t v1, size_t v2, size_t v3)
    : mesh(mesh), v1(v1), v2(v2), v3(v3) {}

BBox Triangle::get_bbox() const {
	// TODO (PathTracer):
	// compute the bounding box of the triangle
	Vector3D p0 = mesh->positions[v1];
	Vector3D p1 = mesh->positions[v2];
	Vector3D p2 = mesh->positions[v3];

	double minX = std::min(p0.x, std::min(p1.x, p2.x));
	double minY = std::min(p0.y, std::min(p1.y, p2.y));
	double minZ = std::min(p0.z, std::min(p1.z, p2.z));
	double maxX = std::max(p0.x, std::max(p1.x, p2.x));
	double maxY = std::max(p0.y, std::max(p1.y, p2.y));
	double maxZ = std::max(p0.z, std::max(p1.z, p2.z));

	return BBox(minX, minY, minZ, maxX, maxY, maxZ);
}

bool Triangle::intersect(const Ray& r) const {
  // TODO (PathTracer): implement ray-triangle intersection
	Vector3D o = r.o;
	Vector3D d = r.d;
	Vector3D p0 = mesh->positions[v1];
	Vector3D p1 = mesh->positions[v2];
	Vector3D p2 = mesh->positions[v3];

	Vector3D e1 = p1 - p0;
	Vector3D e2 = p2 - p0;
	Vector3D s = o - p0;

	Vector3D e1xd = cross(e1, d);
	double det = dot(e1xd,e2);
	if (det < 0.00001) return false;

	Vector3D sxe2 = cross(s, e2);
	double u = -dot(sxe2, d) / det;
	if (u < 0.0 || u > 1.0) return false;

	double v = dot(e1xd, s) / det;
	if (v < 0.0 || u + v > 1.0) return false;

	double t = -dot(sxe2, e1) / det;
	if (t > r.max_t || t < r.min_t) return false;

	r.max_t = t;
	return true;
}

bool Triangle::intersect(const Ray& r, Intersection* isect) const {
	// TODO (PathTracer):
	// implement ray-triangle intersection. When an intersection takes
	// place, the Intersection data should be updated accordingly

	Vector3D o = r.o;
	Vector3D d = r.d;
	Vector3D p0 = mesh->positions[v1];
	Vector3D p1 = mesh->positions[v2];
	Vector3D p2 = mesh->positions[v3];

	Vector3D e1 = p1 - p0;
	Vector3D e2 = p2 - p0;
	Vector3D s = o - p0;

	Vector3D e1xd = cross(e1, d);
	double det = dot(e1xd, e2);

	if (det < 0.0) {
		s *= -1;
		det *= -1;
	}
	if (det < 0.00001) return false;

	Vector3D sxe2 = cross(s, e2);
	double u = -dot(sxe2, d) / det;
	if (u < 0.0 || u > 1.0) return false;

	double v = dot(e1xd, s) / det;
	if (v < 0.0 || u + v > 1.0) return false;

	double t = -dot(sxe2, e1) / det;
	if (t > r.max_t || t < r.min_t) return false;

	r.max_t = t;

	isect->t = t;
	isect->n = (1.0 - u - v) * mesh->normals[v1] + u * mesh->normals[v2] + v * mesh->normals[v3];
	isect->n = isect->n.unit();
	isect->bsdf = mesh->get_bsdf();
	isect->primitive = this;

	if (dot(isect->n, d) > 0.0) isect->n = -isect->n;

	return true;
}

void Triangle::draw(const Color& c) const {
  glColor4f(c.r, c.g, c.b, c.a);
  glBegin(GL_TRIANGLES);
  glVertex3d(mesh->positions[v1].x, mesh->positions[v1].y,
             mesh->positions[v1].z);
  glVertex3d(mesh->positions[v2].x, mesh->positions[v2].y,
             mesh->positions[v2].z);
  glVertex3d(mesh->positions[v3].x, mesh->positions[v3].y,
             mesh->positions[v3].z);
  glEnd();
}

void Triangle::drawOutline(const Color& c) const {
  glColor4f(c.r, c.g, c.b, c.a);
  glBegin(GL_LINE_LOOP);
  glVertex3d(mesh->positions[v1].x, mesh->positions[v1].y,
             mesh->positions[v1].z);
  glVertex3d(mesh->positions[v2].x, mesh->positions[v2].y,
             mesh->positions[v2].z);
  glVertex3d(mesh->positions[v3].x, mesh->positions[v3].y,
             mesh->positions[v3].z);
  glEnd();
}

}  // namespace StaticScene
}  // namespace CMU462
