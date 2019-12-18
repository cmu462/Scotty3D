#include "sphere.h"

#include <cmath>

#include "../bsdf.h"
#include "../misc/sphere_drawing.h"

namespace CMU462 {
namespace StaticScene {

	bool Sphere::test(const Ray& ray, double& t1, double& t2) const {
		// TODO (PathTracer):
		// Implement ray - sphere intersection test.
		// Return true if there are intersections and writing the
		// smaller of the two intersection times in t1 and the larger in t2.
		Vector3D distance = ray.o - o;
		double projection = dot(distance, ray.d);
		Vector3D perpendicular = distance - projection * ray.d;
		if (perpendicular.norm2() > r2) return false;

		double a = dot(ray.d, ray.d);
		double b = 2 * dot(distance, ray.d);
		double c = dot(distance, distance) - r2;

		t2 = (-b + sqrt(b*b - 4 * a*c)) / 2 / a;
		t1 = (-b - sqrt(b*b - 4 * a*c)) / 2 / a;
		return true;
	}

	bool Sphere::intersect(const Ray& ray) const {
		// TODO (PathTracer):
		// Implement ray - sphere intersection.
		// Note that you might want to use the the Sphere::test helper here.
		double t1, t2;
		bool result = test(ray, t1, t2);
		if (result && t1 <= ray.max_t && t1 >= ray.min_t) {
			ray.max_t = t1;
			return true;
		}
		else return false;
	}

	bool Sphere::intersect(const Ray& ray, Intersection* isect) const {
		// TODO (PathTracer):
		// Implement ray - sphere intersection.
		// Note again that you might want to use the the Sphere::test helper here.
		// When an intersection takes place, the Intersection data should be updated
		// correspondingly.
		double t1, t2;
		bool result = test(ray, t1, t2);
		if (result && t1 <= ray.max_t && t1 >= ray.min_t) {
			ray.max_t = t1;

			isect->t = t1;
			isect->n = normal(ray.o+ray.d*t1);
			isect->bsdf = get_bsdf();
			isect->primitive = this;

			return true;
		}
		else return false;
	}

void Sphere::draw(const Color& c) const { Misc::draw_sphere_opengl(o, r, c); }

void Sphere::drawOutline(const Color& c) const {
  // Misc::draw_sphere_opengl(o, r, c);
}

}  // namespace StaticScene
}  // namespace CMU462
