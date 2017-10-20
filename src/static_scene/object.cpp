#include "object.h"
#include "sphere.h"
#include "triangle.h"

#include <vector>
#include <iostream>
#include <unordered_map>

using std::vector;
using std::unordered_map;

namespace CMU462 {
namespace StaticScene {

// Mesh object //

Mesh::Mesh(const HalfedgeMesh& mesh, BSDF* bsdf) {
  // triangulate mesh before sending to visualization or render mode
  HalfedgeMesh _mesh(mesh);

  // Triangulate if needed
  bool triNeeded = false;
  for (auto f = mesh.facesBegin(); f != mesh.facesEnd(); f++) {
    if(f->degree() != 3) {
      triNeeded = true;
    }
  }
  if(triNeeded) {
    _mesh.triangulate();
  }

  unordered_map<const Vertex*, int> vertexLabels;
  vector<const Vertex*> verts;

  size_t vertexI = 0;
  for (VertexCIter it = _mesh.verticesBegin(); it != _mesh.verticesEnd();
       it++) {
    const Vertex* v = &*it;
    verts.push_back(v);
    vertexLabels[v] = vertexI;
    vertexI++;
  }

  positions = new Vector3D[vertexI];
  normals = new Vector3D[vertexI];
  for (int i = 0; i < vertexI; i++) {
    positions[i] = verts[i]->position;
    normals[i] = verts[i]->normal();
  }

  for (FaceCIter f = _mesh.facesBegin(); f != _mesh.facesEnd(); f++) {
    HalfedgeCIter h = f->halfedge();
    indices.push_back(vertexLabels[&*h->vertex()]);
    indices.push_back(vertexLabels[&*h->next()->vertex()]);
    indices.push_back(vertexLabels[&*h->next()->next()->vertex()]);
  }

  this->bsdf = bsdf;
}

vector<Primitive*> Mesh::get_primitives() const {
  vector<Primitive*> primitives;
  size_t num_triangles = indices.size() / 3;
  for (size_t i = 0; i < num_triangles; ++i) {
    Triangle* tri = new Triangle(this, indices[i * 3], indices[i * 3 + 1],
                                 indices[i * 3 + 2]);
    primitives.push_back(tri);
  }
  return primitives;
}

BSDF* Mesh::get_bsdf() const { return bsdf; }

// Sphere object //

SphereObject::SphereObject(const Vector3D& o, double r, BSDF* bsdf) {
  this->o = o;
  this->r = r;
  this->bsdf = bsdf;
}

std::vector<Primitive*> SphereObject::get_primitives() const {
  std::vector<Primitive*> primitives;
  primitives.push_back(new Sphere(this, o, r));
  return primitives;
}

BSDF* SphereObject::get_bsdf() const { return bsdf; }

}  // namespace StaticScene
}  // namespace CMU462
