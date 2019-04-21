#include "widgets.h"
#include "mesh.h"
#include "skeleton.h"
#include "joint.h"

#include <sstream>
using std::ostringstream;

using namespace CMU462;
using namespace DynamicScene;

XFormWidget::XFormWidget() {
  target.object = nullptr;
  target.element = nullptr;
  objectMode = false;
  transformedMode = false;
  poseMode = false;
  mode = lastMode = Mode::Translate;
}

void XFormWidget::enterObjectMode() { objectMode = true; }

void XFormWidget::exitObjectMode() { objectMode = false; }

void CMU462::DynamicScene::XFormWidget::enterPoseMode() {
  poseMode = true;
}

void CMU462::DynamicScene::XFormWidget::exitPoseMode() { poseMode = false; }

void XFormWidget::enterTransformedMode() { transformedMode = true; }

void XFormWidget::exitTransformedMode() { transformedMode = false; }

void XFormWidget::setTarget(Selection& _target) {
  auto originalObj = target.object;
  target = _target;
  if (objectMode) {
    if (target.object == this) target.object = originalObj;
    target.element = nullptr;
  }

  updateGeometry();
}

void XFormWidget::onMouseReleased() {
  isTransforming = false;
}

void XFormWidget::setTranslate() {
  if (lastMode != Mode::Translate) {
    lastMode = mode;
  }
  mode = Mode::Translate;
}

void XFormWidget::setRotate() {
  if (lastMode != Mode::Rotate) {
    lastMode = mode;
  }
  mode = Mode::Rotate;
}

void XFormWidget::setScale() {
  if (lastMode != Mode::Scale) {
    lastMode = mode;
  }
  mode = Mode::Scale;
}

void XFormWidget::cycleMode() {
  if (poseMode) return;

  if (mode == Mode::Translate)
    mode = Mode::Rotate;
  else if (mode == Mode::Rotate)
    mode = Mode::Scale;
  else if (mode == Mode::Scale)
    mode = Mode::Translate;
}

void XFormWidget::restoreLastMode() { mode = lastMode; }

void XFormWidget::setClickPosition(const Vector2D& position) {
  clickPosition = position;
}

void XFormWidget::set_draw_styles(DrawStyle* defaultStyle,
                                  DrawStyle* hoveredStyle,
                                  DrawStyle* selectedStyle) {
  this->defaultStyle = defaultStyle;
  this->hoveredStyle = hoveredStyle;
  this->selectedStyle = selectedStyle;
}

static inline void apply_parent_transform(Selection target) {
  // Only apply here if we are selecting an element inside of a
  // transformed Mesh.  i.e. element != null && object != null
  if (target.element != nullptr && target.object != nullptr) {
    auto pos = target.object->position;
    auto rot = target.object->rotation;
    glTranslated(pos.x, pos.y, pos.z);
    glRotated(rot.z, 0, 0, 1);
    glRotated(rot.y, 0, 1, 0);
    glRotated(rot.x, 1, 0, 0);
  }
}

void XFormWidget::draw() {
  if (target.object == nullptr) return;

  vector<GLubyte> red = {255, 0, 0, 64};
  vector<GLubyte> green = {0, 255, 0, 64};
  vector<GLubyte> blue = {0, 0, 255, 64};
  vector<GLubyte> white = {255, 255, 255, 64};

  if (target.axis == Selection::Axis::X) {
    red[1] = red[2] = 128;
  }
  if (target.axis == Selection::Axis::Y) {
    green[0] = green[2] = 64;
    green[1] = 196;
  }
  if (target.axis == Selection::Axis::Z) {
    blue[0] = blue[1] = 128;
  }
  if (target.axis == Selection::Axis::Center) {
    white[0] = white[1] = white[2] = 192;
  }

  axisColors.resize(4);
  axisColors[0] = red;
  axisColors[1] = green;
  axisColors[2] = blue;
  axisColors[3] = white;

  glPushMatrix();
  glDisable(GL_LIGHTING);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  drawHandles();

  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  drawHandles();
  glEnable(GL_LIGHTING);
  glPopMatrix();
}

void XFormWidget::drawHandles() const {
  glPushMatrix();
  apply_parent_transform(target);
  switch (mode) {
    case Mode::Translate:
      drawTranslateHandles();
      break;
    case Mode::Rotate:
      drawRotateHandles();
      break;
    case Mode::Scale:
      drawScaleHandles();
      break;
    default:
      break;
  }
  glPopMatrix();
}

BBox XFormWidget::get_bbox() { return bounds; }

Info XFormWidget::getInfo() {
  Info info;

  if (target.element != nullptr) {
    info = target.element->getInfo();
    return info;
  }

  if (target.object != nullptr) {
    info = target.object->getInfo();
    return info;
  }

  info.push_back("TRANSFORMATION");
  return info;
}

void XFormWidget::directionalTransform(Vector3D& p, vector<int> I, Vector3D c,
                                       double x, double y, double dx, double dy,
                                       const Matrix4x4& modelViewProj) {
  // For methods that use only one axis, find the active axis.
  int i = 0;
  for (int p = 0; p < 3; p++) {
    if (I[p]) {
      i = p;
      break;
    }
  }

  // Build change of basis for axis-aligned frame
  Matrix3x3 E;
  E[0] = axes[0];
  E[1] = axes[1];
  E[2] = axes[2];

  if (mode == Mode::Rotate) {
    // // For rotation, the speed of rotation will depend on how well the cursor
    // // motion lines up with the tangent of the circle at the current point.
    // Vector4D d( c, 1. );
    // d = modelViewProj * d;
    // Vector2D q0( d.x, d.y );
    // Vector2D q1( x, y );
    // Vector2D dq( dx, dy );
    // Vector2D r = (q1-q0).unit();
    // double m = dot( r, dq );

    Vector4D d(c, 1.);
    d = modelViewProj * d;
    d.x /= d.w;
    d.y /= d.w;
    d.z /= d.w;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    Vector2D A(x, y);
    Vector2D B = clickPosition;
    Vector2D C(viewport[2] * (d.x + 1.) / 2.,
               viewport[3] * (1. - (d.y + 1.) / 2.));
    Vector2D U = (A - C).unit();
    Vector2D V = (B - C).unit();
    double m = atan2(cross(U, V), dot(U, V));

    Matrix3x3 R;
    if (i == 0) {
      R(0, 0) = 1.;
      R(0, 1) = 0.;
      R(0, 2) = 0.;
      R(1, 0) = 0.;
      R(1, 1) = cos(m);
      R(1, 2) = sin(m);
      R(2, 0) = 0.;
      R(2, 1) = -sin(m);
      R(2, 2) = cos(m);
    } else if (i == 1) {
      R(0, 0) = cos(m);
      R(0, 1) = 0.;
      R(0, 2) = -sin(m);
      R(1, 0) = 0.;
      R(1, 1) = 1.;
      R(1, 2) = 0.;
      R(2, 0) = sin(m);
      R(2, 1) = 0.;
      R(2, 2) = cos(m);
    } else if (i == 2) {
      R(0, 0) = cos(m);
      R(0, 1) = -sin(m);
      R(0, 2) = 0.;
      R(1, 0) = sin(m);
      R(1, 1) = cos(m);
      R(1, 2) = 0.;
      R(2, 0) = 0.;
      R(2, 1) = 0.;
      R(2, 2) = 1.;
    }

    p -= c;
    p = E * (R * (E.T() * p));
    p += c;
  }

  // For translation and scale, the speed of transformation will depend on how
  // well the cursor motion lines up with the selected axis in screen space.
  Vector3D u = E[i];
  Vector4D v(u, 0.);
  v = modelViewProj * v;
  double M = sqrt(v.x * v.x + v.y * v.y);
  v.x /= M;
  v.y /= M;
  auto const sign = [](double d) {return d >= 0 ? 1.0 : -1.0;};
  double m = sign(dx * v.x + dy * v.y) *                              // direction of transformation determined by dot product
                sqrt((dx * v.x)*(dx * v.x) + (dy * v.y)*(dy * v.y));  // speed of transformation determined by length of skewed vector

  if (I[0] == 1 && I[1] == 1 && I[2] == 1) {
    m = dx + dy;
  }

  if (mode == Mode::Translate) {
    Vector4D q(c, 1.);

    // Transform into clip space
    q = modelViewProj * q;
    double w = q.w;
    q /= w;

    q.x += m * v.x;
    q.y += m * v.y;
    q.z += m * v.z;
    q.w += m * v.w;

    // Transform back into model space
    q *= w;
    q = modelViewProj.inv() * q;

    // Project motion onto axis of translation
    double s = dot(q.to3D() - c, u);
    p += s * u;
  } else {
    Matrix3x3 B;
    B.zero();
    B(0, 0) = I[0] ? exp(2. * m) : 1.;
    B(1, 1) = I[1] ? exp(2. * m) : 1.;
    B(2, 2) = I[2] ? exp(2. * m) : 1.;

    p -= c;
    p = E * (B * (E.T() * p));
    p += c;
  }
}

void XFormWidget::drag(double x, double y, double dx, double dy,
                       const Matrix4x4& modelViewProj) {
  if (target.axis == Selection::Axis::None) return;
  if (target.object == nullptr) return;
  if (target.element == nullptr && !objectMode) return;

  if (objectMode) {
    Joint *j = dynamic_cast<Joint *>(target.object);
    bool isJoint = j != nullptr;

    Vector3D jpos;
    Vector3D* update;
    switch (mode) {
      case Mode::Translate:
        if(isJoint && j->skeleton->root != j) {
          jpos = j->getEndPosInWorld();
          update = &jpos;
        } else {
          update = &target.object->position;
        }
        break;
      case Mode::Scale:
        update = &target.object->scale;
        break;
      case Mode::Rotate:
        update = &target.object->rotation;
        break;
    }

    if (mode == Mode::Rotate) {
      double winX, winY, winZ;
      double model[16], proj[16];
      int view[4];
      glGetDoublev(GL_MODELVIEW_MATRIX, model);
      glGetDoublev(GL_PROJECTION_MATRIX, proj);
      glGetIntegerv(GL_VIEWPORT, view);
      gluProject(center.x, center.y, center.z, model, proj, view, &winX, &winY,
                 &winZ);
      winY = view[3] - winY;
      double theta = atan2(winY - y, x - winX) / PI * 180;
      switch (target.axis) {
        case Selection::Axis::X:
          update->x = theta;
          isTransforming = true;
          break;
        case Selection::Axis::Y:
          update->y = theta;
          isTransforming = true;
          break;
        case Selection::Axis::Z:
          update->z = theta;
          isTransforming = true;
          break;
        default:
          // Do nothing
          break;
      }
    } else if (mode == Mode::Translate &&
               target.axis == Selection::Axis::Center) {
      Vector4D q(*update, 1.);
      q = modelViewProj * q;
      double w = q.w;
      q /= w;
      // Shift by (dx, dy).
      q.x += dx;
      q.y += dy;
      // Transform back into model s*updateace
      q *= w;
      q = modelViewProj.inv() * q;

      if(!isJoint)
        *update = q.to3D();
      else if(j->skeleton->root == j)
        j->position = q.to3D();
      else
        j->axis += q.to3D() - jpos;
      
      isTransforming = true;
    } else {
      vector<int> I = {0, 0, 0};
      switch (target.axis) {
        case Selection::Axis::X:
          I[0] = 1;
          isTransforming = true;
          break;
        case Selection::Axis::Y:
          I[1] = 1;
          isTransforming = true;
          break;
        case Selection::Axis::Z:
          I[2] = 1;
          isTransforming = true;
          break;
        case Selection::Axis::Center:
          I[0] = I[1] = I[2] = 1;
          isTransforming = true;
          break;
        default:
          break;
      }
      Vector3D c = mode == Mode::Translate ? center : Vector3D(0, 0, 0);

      if(!isJoint || j->skeleton->root == j)
        directionalTransform(*update, I, c, x, y, dx, dy, modelViewProj);
      else {
        Vector3D npos = jpos;
        directionalTransform(npos, I, c, x, y, dx, dy, modelViewProj);
        j->axis += npos - jpos;
      }
      
    }

    return;
  }

  if (mode == Mode::Translate && target.axis == Selection::Axis::Center) {
    target.element->translate(dx, dy, modelViewProj);
    isTransforming = true;
    // TODO uniform scale, free rotate
    return;
  }

  Vertex* v = target.element->getVertex();
  Edge* e = target.element->getEdge();
  Face* f = target.element->getFace();

  center = target.element->centroid();

  vector<int> I = {0, 0, 0};
  switch (target.axis) {
    case Selection::Axis::X:
      I[0] = 1;
      isTransforming = true;
      break;
    case Selection::Axis::Y:
      I[1] = 1;
      isTransforming = true;
      break;
    case Selection::Axis::Z:
      I[2] = 1;
      isTransforming = true;
      break;
    case Selection::Axis::Center:
      I[0] = I[1] = I[2] = 1;
      isTransforming = true;
      break;
    default:
      break;
  }

  if (v) {
    directionalTransform(v->position, I, center, x, y, dx, dy, modelViewProj);
  } else if (e) {
    HalfedgeIter h = e->halfedge();
    do {
      directionalTransform(h->vertex()->position, I, center, x, y, dx, dy,
                           modelViewProj);
      h = h->twin();
    } while (h != e->halfedge());
  } else if (f) {
    HalfedgeIter h = f->halfedge();
    do {
      directionalTransform(h->vertex()->position, I, center, x, y, dx, dy,
                           modelViewProj);
      h = h->next();
    } while (h != f->halfedge());
  }

  clickPosition = Vector2D(x, y);
}

StaticScene::SceneObject* XFormWidget::get_static_object() { return nullptr; }

void XFormWidget::draw_pick(int& pickID, bool transformed) {
  if (target.object == nullptr) return;

  pickIDToAxis[pickID + 0] = Selection::Axis::X;
  pickIDToAxis[pickID + 1] = Selection::Axis::Y;
  pickIDToAxis[pickID + 2] = Selection::Axis::Z;
  pickIDToAxis[pickID + 3] = Selection::Axis::Center;

  vector<GLubyte> Xcolor(4), Ycolor(4), Zcolor(4), Ccolor(4);
  IndexToRGB(pickID + 0, Xcolor[0], Xcolor[1], Xcolor[2]);
  IndexToRGB(pickID + 1, Ycolor[0], Ycolor[1], Ycolor[2]);
  IndexToRGB(pickID + 2, Zcolor[0], Zcolor[1], Zcolor[2]);
  IndexToRGB(pickID + 3, Ccolor[0], Ccolor[1], Ccolor[2]);
  Xcolor[3] = Ycolor[3] = Zcolor[3] = Ccolor[3] = 64;

  axisColors.resize(4);
  axisColors[0] = Xcolor;
  axisColors[1] = Ycolor;
  axisColors[2] = Zcolor;
  axisColors[3] = Ccolor;

  glPushMatrix();

  apply_parent_transform(target);

  switch (mode) {
    case Mode::Translate:
      drawTranslateHandles();
      break;
    case Mode::Rotate:
      drawRotateHandles();
      break;
    case Mode::Scale:
      drawScaleHandles();
      break;
    default:
      break;
  }
  glPopMatrix();

  pickID += 4;
}

void XFormWidget::setSelection(int pickID, Selection& selection) {
  // Don't change selection if the mouse is down and we are transforming
  if(isTransforming)
    return;

  if (pickIDToAxis.find(pickID) != pickIDToAxis.end()) {
    selection.clear();
    selection.object = this;
    selection.element = nullptr;
    selection.axis = target.axis = pickIDToAxis[pickID];
  } else {
    target.axis = Selection::Axis::None;
  }
}

bool XFormWidget::getIsTransforming() const {
  return isTransforming;
}

void XFormWidget::updateGeometry() {
  if (target.object == nullptr) return;
  axes.resize(3);
  if (objectMode) {
    Joint* joint = dynamic_cast<Joint*>(target.object);
    if (joint != nullptr) {
      if(poseMode)
        center = joint->getBasePosInWorld();
      else
        center = joint->getEndPosInWorld();
    } else
      center = target.object->position;

    if (poseMode && joint && joint == joint->skeleton->root) {
      bounds = joint->skeleton->mesh->get_bbox();
    } else if(!poseMode && joint) {
      auto p = joint->position;
      auto e = joint->skeleton->mesh->get_bbox().extent * 0.15f;
      bounds = BBox(p-e, p+e);
    }
    else bounds = target.object->get_bbox();

    if (poseMode && joint != nullptr && joint != joint->skeleton->root) {
      joint->getAxes(axes);
    } else {
      axes[0] = Vector3D(1., 0., 0.);
      axes[1] = Vector3D(0., 1., 0.);
      axes[2] = Vector3D(0., 0., 1.);
    }
  } else {
    if (target.element == nullptr) return;
    if (transformedMode) {
      center = target.element->centroid() + target.object->position;
    } else {
      center = target.element->centroid();
    }
    bounds = target.element->bounds();
    target.element->getAxes(axes);
  }
}

void XFormWidget::drawTranslateHandles() const {
  const double arrowSize = .15;
  const int nSides = 8;

  if (target.object == nullptr) return;

  int startAxis = (transformedMode && !objectMode) ? 2 : 0;

  Vector3D c = center;
  double r = (bounds.max - bounds.min).norm() / 2.;

  // Draw arrow stems
  glLineWidth(8.);
  glBegin(GL_LINES);
  for (int i = startAxis; i < 3; i++) {
    Vector3D b = c + (1. - arrowSize) * r * axes[i];

    glColor4ubv(&axisColors[i][0]);

    glVertex3dv(&c.x);
    glVertex3dv(&b.x);
  }
  glEnd();

  // Draw arrow heads
  glBegin(GL_TRIANGLES);
  for (int i = startAxis; i < 3; i++) {
    Vector3D a = c + r * axes[i];
    Vector3D b = c + (1. - arrowSize) * r * axes[i];
    Vector3D e0 = r * .5 * arrowSize * axes[(i + 1) % 3];
    Vector3D e1 = r * .5 * arrowSize * axes[(i + 2) % 3];

    glColor4ubv(&axisColors[i][0]);

    for (int j = 0; j < nSides; j++) {
      double theta1 = 2. * M_PI * (double)j / (double)nSides;
      double theta2 = 2. * M_PI * (double)(j + 1) / (double)nSides;
      Vector3D p0 = b + cos(theta1) * e0 - sin(theta1) * e1;
      Vector3D p1 = b + cos(theta2) * e0 - sin(theta2) * e1;

      glVertex3dv(&a.x);
      glVertex3dv(&p0.x);
      glVertex3dv(&p1.x);

      glVertex3dv(&p0.x);
      glVertex3dv(&b.x);
      glVertex3dv(&p1.x);
    }
  }
  glEnd();

  if (!transformedMode || objectMode) drawCenterHandle();
  glLineWidth(1.);
}

void XFormWidget::drawRotateHandles() const {
  const double boxSize = .2;
  const int nSides = 64;

  if (target.object == nullptr) return;

  Vector3D c = center;
  double r = (bounds.max - bounds.min).norm() / 2.;

  glLineWidth(8.);
  glBegin(GL_LINES);
  for (int i = 0; i < 3; i++) {
    Vector3D e1 = axes[(i + 1) % 3];
    Vector3D e2 = axes[(i + 2) % 3];

    glColor4ubv(&axisColors[i][0]);

    for (int j = 0; j < nSides; j++) {
      double theta1 = 2. * M_PI * (double)(j + 0) / (double)nSides;
      double theta2 = 2. * M_PI * (double)(j + 1) / (double)nSides;

      Vector3D q1 = c + r * (cos(theta1) * e1 - sin(theta1) * e2);
      Vector3D q2 = c + r * (cos(theta2) * e1 - sin(theta2) * e2);

      glVertex3dv(&q1.x);
      glVertex3dv(&q2.x);
    }
  }
  glEnd();

  drawCenterHandle();

  glLineWidth(1.);
}

void XFormWidget::drawScaleHandles() const {
  const double boxSize = .2;

  if (target.object == nullptr) return;

  Vector3D c = center;
  double r = (bounds.max - bounds.min).norm() / 2.;

  // Draw arrow stems
  glLineWidth(8.);
  glBegin(GL_LINES);
  for (int i = 0; i < 3; i++) {
    Vector3D b = c + (1. - boxSize / 2.) * r * axes[i];

    glColor4ubv(&axisColors[i][0]);

    glVertex3dv(&c.x);
    glVertex3dv(&b.x);
  }
  glEnd();

  // Draw box heads
  glBegin(GL_QUADS);
  for (int i = 0; i < 3; i++) {
    glColor4ubv(&axisColors[i][0]);
    Vector3D q = c + r * (1. - boxSize) * axes[i];
    Vector3D q000 = q + .5 * r * boxSize * (axes[0] - axes[1] - axes[2]);
    Vector3D q001 = q + .5 * r * boxSize * (axes[0] - axes[1] + axes[2]);
    Vector3D q010 = q + .5 * r * boxSize * (axes[0] + axes[1] - axes[2]);
    Vector3D q011 = q + .5 * r * boxSize * (axes[0] + axes[1] + axes[2]);
    Vector3D q100 = q + .5 * r * boxSize * (-axes[0] - axes[1] - axes[2]);
    Vector3D q101 = q + .5 * r * boxSize * (-axes[0] - axes[1] + axes[2]);
    Vector3D q110 = q + .5 * r * boxSize * (-axes[0] + axes[1] - axes[2]);
    Vector3D q111 = q + .5 * r * boxSize * (-axes[0] + axes[1] + axes[2]);

    glVertex3dv(&q000.x);
    glVertex3dv(&q001.x);
    glVertex3dv(&q011.x);
    glVertex3dv(&q010.x);
    glVertex3dv(&q100.x);
    glVertex3dv(&q101.x);
    glVertex3dv(&q111.x);
    glVertex3dv(&q110.x);
    glVertex3dv(&q000.x);
    glVertex3dv(&q010.x);
    glVertex3dv(&q110.x);
    glVertex3dv(&q100.x);
    glVertex3dv(&q001.x);
    glVertex3dv(&q011.x);
    glVertex3dv(&q111.x);
    glVertex3dv(&q101.x);
    glVertex3dv(&q000.x);
    glVertex3dv(&q100.x);
    glVertex3dv(&q101.x);
    glVertex3dv(&q001.x);
    glVertex3dv(&q010.x);
    glVertex3dv(&q110.x);
    glVertex3dv(&q111.x);
    glVertex3dv(&q011.x);
  }
  glEnd();

  drawCenterHandle();

  glLineWidth(1.);
}

void XFormWidget::drawCenterHandle() const {
  const double handleSize = .15;

  Vector3D c = center;

  double r = (bounds.max - bounds.min).norm() / 2.;

  // Draw center box
  glBegin(GL_QUADS);
  glColor4ubv(&axisColors[3][0]);
  for (int i = 0; i < 3; i++) {
    for (double s = -1.; s <= 1.; s += 2.) {
      Vector3D p = c + r * s * handleSize * axes[i];
      Vector3D p00 =
          p + r * handleSize * (-axes[(i + 1) % 3] - axes[(i + 2) % 3]);
      Vector3D p01 =
          p + r * handleSize * (-axes[(i + 1) % 3] + axes[(i + 2) % 3]);
      Vector3D p10 =
          p + r * handleSize * (axes[(i + 1) % 3] - axes[(i + 2) % 3]);
      Vector3D p11 =
          p + r * handleSize * (axes[(i + 1) % 3] + axes[(i + 2) % 3]);

      glVertex3dv(&p00.x);
      glVertex3dv(&p01.x);
      glVertex3dv(&p11.x);
      glVertex3dv(&p10.x);
    }
  }
  glEnd();
}
