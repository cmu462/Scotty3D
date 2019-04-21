#include "scene.h"
#include "joint.h"
#include "../halfEdgeMesh.h"
#include "mesh.h"
#include "widgets.h"
#include <fstream>

using std::cout;
using std::endl;

namespace CMU462 {
namespace DynamicScene {

Scene::Scene(std::vector<SceneObject *> _objects,
             std::vector<SceneLight *> _lights) {
  for (int i = 0; i < _objects.size(); i++) {
    _objects[i]->scene = this;
    objects.insert(_objects[i]);
  }

  for (int i = 0; i < _lights.size(); i++) {
    lights.insert(_lights[i]);
  }

  elementTransform = new XFormWidget();
}

Scene::~Scene() {
  if (elementTransform != nullptr) {
    delete elementTransform;
  }
}

BBox Scene::get_bbox() {
  BBox bbox;
  for (SceneObject *obj : objects) {
    bbox.expand(obj->get_bbox());
  }
  return bbox;
}

bool Scene::addObject(SceneObject *o) {
  auto i = objects.find(o);
  if (i != objects.end()) {
    return false;
  }

  o->scene = this;
  o->set_draw_styles(defaultStyle, hoveredStyle, selectedStyle);
  objects.insert(o);
  return true;
}

bool Scene::removeObject(SceneObject *o) {
  auto i = objects.find(o);
  if (i == objects.end()) {
    return false;
  }

  objects.erase(o);
  return true;
}

void Scene::set_draw_styles(DrawStyle *defaultStyle, DrawStyle *hoveredStyle,
                            DrawStyle *selectedStyle) {
  this->defaultStyle = defaultStyle;
  this->hoveredStyle = hoveredStyle;
  this->selectedStyle = selectedStyle;
  for (SceneObject *obj : objects) {
    obj->set_draw_styles(defaultStyle, hoveredStyle, selectedStyle);
  }
}

void Scene::render_in_opengl() {
  for (SceneObject *obj : objects) {
    if (obj->isVisible && !obj->isGhosted) {
      obj->draw();
    }
  }

  for (SceneObject *obj : objects) {
    if (obj->isVisible && obj->isGhosted) {
      obj->drawGhost();
    }
  }
}

void Scene::render_splines_at(double time, bool pretty, bool useCapsuleRadius, bool depth_only) {
  // Update splines
  for (SceneObject *obj : objects) {
    obj->position = obj->positions.evaluate(time);
    obj->rotation = obj->rotations.evaluate(time);
    obj->scale = obj->scales.evaluate(time);
  }

  // Perform linear blend skinning before rendering
  for (SceneObject *obj : objects) {
    Mesh *mesh = dynamic_cast<Mesh *>(obj);
    if (mesh != nullptr) {
      mesh->linearBlendSkinning(useCapsuleRadius);
    }
  }

  if(depth_only) {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    //glDisable(GL_ALPHA_TEST);
    GLfloat white[4] = {1., 1., 1., 1.};
    for (SceneObject *obj : objects) {
      if(obj->isVisible)
        (pretty) ? (obj->draw_pretty()) : (obj->draw());
      //glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    }
    //glEnable(GL_ALPHA_TEST);
    return;
  }

  // draw the scene twice using alpha test
  for (SceneObject *obj : objects) {
    GLfloat white[4] = {1., 1., 1., 1.};
    if (obj->isVisible && !obj->isGhosted) {
      (pretty) ? (obj->draw_pretty()) : (obj->draw());
    } else if (obj->isVisible && obj->isGhosted) {
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GEQUAL, 1);
      obj->drawGhost();
      glDisable(GL_ALPHA_TEST);
    }
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
  }

  for (SceneObject *obj : objects) {
    if (obj->isVisible && obj->isGhosted) {
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_LESS, 1);
      glDepthMask(GL_FALSE);
      obj->drawGhost();
      glDisable(GL_ALPHA_TEST);
      glDepthMask(GL_TRUE);
    }
  }
}

void Scene::draw_spline_curves(Timeline &timeline) {
  const double nSamplesPerTick = 1.;
  const double tangentLengthScale = 5.;
  const double controlPointScale = 10.;

  glPushAttrib(GL_ALL_ATTRIB_BITS);

  glEnable(GL_BLEND);
  glDisable(GL_LIGHTING);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // use round (rather than square) points
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

  for (SceneObject *obj : objects) {
    bool selected = (obj == this->selected.object);
    double alpha = selected ? 1. : .5;
    Spline<Vector3D> &spline = obj->positions;
    // approximate spline by line segments
    const double maxTime = timeline.getMaxFrame();
    glLineWidth(3.);
    glColor4f(1., 1., 1., alpha);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_STRIP);
    for (double t = 0; t <= maxTime; t += 1. / nSamplesPerTick) {
      Vector3D p = spline(t);

      /* Zach: This is all complicated stuff to make the spline appear as
       * shifting
       * in hue as time progresses, to give an impression of how fast it's going
       */
      double frac = t / maxTime;
      double lum = 0.5;
      double sat = 1.0;
      double tmp1 = (lum < 0.5) ? (lum * (1.0 + sat)) : (lum + sat - lum * sat);
      double tmp2 = 2 * lum - tmp1;
      double hue = frac;
      double tmpR = hue + 0.333;
      double tmpG = hue;
      double tmpB = hue - 0.333;
      tmpR = (tmpR < 0) ? (tmpR + 1) : ((tmpR > 1) ? (tmpR - 1) : tmpR);
      tmpG = (tmpG < 0) ? (tmpG + 1) : ((tmpG > 1) ? (tmpG - 1) : tmpG);
      tmpB = (tmpB < 0) ? (tmpB + 1) : ((tmpB > 1) ? (tmpB - 1) : tmpB);

      double R = 1.0, G = 1.0, B = 1.0;
      if (6 * tmpR < 1) {
        R = tmp2 + (tmp1 - tmp2) * 6 * tmpR;
      } else if (2 * tmpR < 1) {
        R = tmp1;
      } else if (3 * tmpR < 2) {
        R = tmp2 + (tmp1 - tmp2) * (0.666 - tmpR) * 6;
      } else {
        R = tmp2;
      }

      if (6 * tmpG < 1) {
        G = tmp2 + (tmp1 - tmp2) * 6 * tmpG;
      } else if (2 * tmpG < 1) {
        G = tmp1;
      } else if (3 * tmpG < 2) {
        G = tmp2 + (tmp1 - tmp2) * (0.666 - tmpG) * 6;
      } else {
        G = tmp2;
      }

      if (6 * tmpB < 1) {
        B = tmp2 + (tmp1 - tmp2) * 6 * tmpB;
      } else if (2 * tmpB < 1) {
        B = tmp1;
      } else if (3 * tmpB < 2) {
        B = tmp2 + (tmp1 - tmp2) * (0.666 - tmpB) * 6;
      } else {
        B = tmp2;
      }
      
      int ticks_per_line = 20;
      float color = sqrt(((int)t % ticks_per_line) / (ticks_per_line - 1.0));

      glColor4f(color, color, 1.0, 1.0);
      glVertex3d(p.x, p.y, p.z);
    }
    glEnd();
    glEnable(GL_BLEND);

    if (selected) {
      // draw dot for current position
      double t0 = timeline.getCurrentFrame();
      double fractionThrough = t0 / timeline.getMaxFrame();
      Vector3D p0 = spline(t0);
      glColor4f(1., 1., 1., alpha);
      glPointSize(controlPointScale * 1.5);
      glBegin(GL_POINTS);
      glVertex3d(p0.x, p0.y, p0.z);
      glEnd();
      // draw control points, tangents, and curvature
      for (Spline<Vector3D>::KnotCIter k = spline.knots.begin();
           k != spline.knots.end(); k++) {
        double t = k->first;
        Vector3D P = k->second;                                   // position
        Vector3D T = tangentLengthScale * spline.evaluate(t, 1);  // tangent
        Vector3D A =
            tangentLengthScale * spline.evaluate(t, 2);  // acceleration
        glColor4f(1., 1., 1., alpha);
        glPointSize(controlPointScale);
        glBegin(GL_POINTS);
        glVertex3d(P.x, P.y, P.z);
        glEnd();
      }
    }
  }
  glEnable(GL_LIGHTING);
  glPopAttrib();
}

void Scene::getHoveredObject(const Vector2D &p, bool getElement,
                             bool transformed) {
  // Set the background color to the maximum possible value---this value should
  // be far
  // beyond the maximum pick index, since we have at most 2^(8+8+8) = 16,777,216
  // distinct IDs
  glClearColor(1., 1., 1., 1.);

  // Clear any color values currently in the color buffer---we do not want to
  // use these for
  // picking, since they represent, e.g., shading colors rather than pick IDs.
  // Also clear
  // the depth buffer so that we can use it to determine the closest object
  // under the cursor.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // We want to draw the pick IDs as raw color values; shading functionality
  // like lighting and blending shouldn't interfere.
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glDisable(GL_LIGHTING);
  glDisable(GL_BLEND);

  // Keep track of the number of picking IDs used so far
  int pickID = 0;

  for (auto o : objects) {
    if (o->isPickable) {
      // The implementation of draw_pick MUST increment the
      // pickID for each new pickable element it draws.
      o->draw_pick(pickID, transformed);
    }
  }

  unsigned char color[4];
  glReadPixels(p.x, p.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, color);

  int ID = RGBToIndex(color[0], color[1], color[2]);

  // By default, set hovered object to "none"
  hovered.clear();

  // Determine which element generated this pick ID
  int i = 0;
  for (auto o : objects) {
    // Call the object's method for setting the selection
    // based on the ID.  (This allows the object to set
    // the selection to an element within that particular
    // object type, e.g., for a mesh it can specify that a
    // particular vertex is selected, or for a camera it might
    // specify that a control handle was selected, etc.)
    o->setSelection(ID, hovered);

    i++;
  }

  if (!getElement) {
    // Discard element information
    hovered.element = NULL;
  }

  // Restore any draw state that we disabled above.
  glPopAttrib();
}

bool Scene::has_selection() { return selected.object != nullptr; }

bool Scene::has_hover() { return hovered.object != NULL; }

void Scene::selectNextHalfedge() {
  if (selected.element) {
    Halfedge *h = selected.element->getHalfedge();
    if (h) {
      selected.element = elementAddress(h->next());
    }
  }
}

void Scene::selectTwinHalfedge() {
  if (selected.element) {
    Halfedge *h = selected.element->getHalfedge();
    if (h) {
      selected.element = elementAddress(h->twin());
    }
  }
}

void Scene::selectHalfedge() {
  if (selected.element) {
    Face *f = selected.element->getFace();
    Edge *e = selected.element->getEdge();
    Vertex *v = selected.element->getVertex();
    HalfedgeIter h;
    if (f != nullptr) {
      h = f->halfedge();
    } else if (e != nullptr) {
      h = e->halfedge();
    } else if (v != nullptr) {
      h = v->halfedge();
    } else {
      return;
    }
    selected.element = elementAddress(h);
  }
}

void Scene::triangulateSelection() {
  Mesh *mesh = dynamic_cast<Mesh *>(selected.object);
  if (mesh) {
    mesh->mesh.triangulate();
    clearSelections();
  }
}

void Scene::subdivideSelection(bool useCatmullClark) {
  if (selected.object == nullptr) return;

  Mesh *mesh = dynamic_cast<Mesh *>(selected.object);
  if (mesh) {
    mesh->mesh.subdivideQuad(useCatmullClark);

    // Old elements are invalid
    clearSelections();

    // Select the subdivided mesh again, so that we can keep hitting
    // the same key to get multiple subdivisions (without having to
    // click on a mesh element).
    selected.object = mesh;
    selected.element = elementAddress(mesh->mesh.verticesBegin());
  }
}

void Scene::clearSelections() {
  hovered.clear();
  selected.clear();
  edited.clear();
  elementTransform->target.clear();
}

Info Scene::getSelectionInfo() {
  Info info;

  if (!selected.object) {
    info.push_back("(nothing selected)");
    return info;
  }

  info = selected.object->getInfo();
  return info;
}

void Scene::collapse_selected_element() {
  if (selected.object == nullptr || selected.element == nullptr) return;
  Mesh *m = dynamic_cast<Mesh *>(selected.object);
  if (m) m->collapse_selected_element();
}

void Scene::flip_selected_edge() {
  if (selected.object == nullptr || selected.element == nullptr) return;
  Mesh *m = dynamic_cast<Mesh *>(selected.object);
  if (m) m->flip_selected_edge();
}

void Scene::split_selected_edge() {
  if (selected.object == nullptr || selected.element == nullptr) return;
  Mesh *m = dynamic_cast<Mesh *>(selected.object);
  if (m) m->split_selected_edge();
}

void Scene::erase_selected_element() {
  if (selected.object == nullptr || selected.element == nullptr) return;
  Mesh *m = dynamic_cast<Mesh *>(selected.object);
  if (m) m->erase_selected_element();
}

void Scene::erase_selected_joint() {
  if (selected.object == nullptr) return;
  Joint *j = dynamic_cast<Joint *>(selected.object);
  if (j) {
    j->removeJoint(this);
    selected.clear();
  }
}

void Scene::upsample_selected_mesh() {
  if (selected.object == nullptr || selected.element == nullptr) return;
  Mesh *m = dynamic_cast<Mesh *>(selected.object);
  if (m) m->upsample();
  clearSelections();
}

void Scene::downsample_selected_mesh() {
  if (selected.object == nullptr || selected.element == nullptr) return;
  Mesh *m = dynamic_cast<Mesh *>(selected.object);
  if (m) m->downsample();
  clearSelections();
}

void Scene::resample_selected_mesh() {
  if (selected.object == nullptr || selected.element == nullptr) return;
  Mesh *m = dynamic_cast<Mesh *>(selected.object);
  if (m) m->resample();
  clearSelections();
}

StaticScene::Scene *Scene::get_static_scene() {
  std::vector<StaticScene::SceneObject *> staticObjects;
  std::vector<StaticScene::SceneLight *> staticLights;

  for (SceneObject *obj : objects) {
    auto staticObject = obj->get_static_object();
    if (staticObject != nullptr) staticObjects.push_back(staticObject);
  }
  for (SceneLight *light : lights) {
    staticLights.push_back(light->get_static_light());
  }

  return new StaticScene::Scene(staticObjects, staticLights);
}

StaticScene::Scene *Scene::get_transformed_static_scene(double t) {
  std::vector<StaticScene::SceneObject *> staticObjects;
  std::vector<StaticScene::SceneLight *> staticLights;

  for (SceneObject *obj : objects) {
    auto staticObject = obj->get_transformed_static_object(t);
    if (staticObject != nullptr) staticObjects.push_back(staticObject);
  }
  for (SceneLight *light : lights) {
    staticLights.push_back(light->get_static_light());
  }
  return new StaticScene::Scene(staticObjects, staticLights);
}

void Scene::bevel_selected_element() {
  // Don't re-bevel an element that we're already editing (i.e., beveling)
  if (edited.element == selected.element) return;

  Mesh *mesh = dynamic_cast<Mesh *>(selected.object);
  if (mesh) {
    mesh->bevel_selected_element();
    edited = selected;
  }
}

void Scene::update_bevel_amount(float dx, float dy) {
  Mesh *mesh = dynamic_cast<Mesh *>(edited.object);
  if (mesh) {
    mesh->bevelComputeNewPositions(dx / 500., dy / 500.);
  }
}

Matrix4x4 SceneObject::getTransformation() {
  Vector3D rot = rotation * M_PI / 180;
  return Matrix4x4::translation(position) *
         Matrix4x4::rotation(rot.x, Matrix4x4::Axis::X) *
         Matrix4x4::rotation(rot.y, Matrix4x4::Axis::Y) *
         Matrix4x4::rotation(rot.z, Matrix4x4::Axis::Z) *
         Matrix4x4::scaling(scale);
}

Matrix4x4 SceneObject::getRotation() {
  Vector3D rot = rotation * M_PI / 180;
  return Matrix4x4::rotation(rot.x, Matrix4x4::Axis::X) *
         Matrix4x4::rotation(rot.y, Matrix4x4::Axis::Y) *
         Matrix4x4::rotation(rot.z, Matrix4x4::Axis::Z);
}

}  // namespace DynamicScene
}  // namespace CMU462
