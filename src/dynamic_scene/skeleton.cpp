/*
 * Implementations for Joint Based Skeletons.
 *
 * Started on October 29th, 2015 by Bryce Summers.
 */

#include "mesh.h"
#include "skeleton.h"
#include "joint.h"
#include "CMU462/matrix4x4.h"
#include "CMU462/vector4D.h"

#include "GL/glew.h"

#include <iostream>

using namespace tinyxml2;

#ifndef XMLCheckResult
#define XMLCheckResult(a_eResult)     \
  if (a_eResult != XML_SUCCESS) {     \
    printf("Error: %i\n", a_eResult); \
  }
#endif

namespace CMU462 {
namespace DynamicScene {

Skeleton::Skeleton(Mesh* mesh) {
  this->mesh = mesh;
  joints.clear();
  root = createNewJoint(nullptr, Vector3D(0, 0, 0));
}

void Skeleton::set_draw_styles(DrawStyle* defaultStyle, DrawStyle* hoveredStyle,
                               DrawStyle* selectedStyle) {
  this->defaultStyle = defaultStyle;
  this->hoveredStyle = hoveredStyle;
  this->selectedStyle = selectedStyle;
}

BBox Skeleton::get_bbox() {
  BBox bbox;
  return bbox;
}

Info Skeleton::getInfo() {
  Info info;

  if (!scene || !scene->selected.element) {
    info.push_back("SKELETON");
  } else {
    info = scene->selected.element->getInfo();
  }

  return info;
}

void Skeleton::drag(double x, double y, double dx, double dy,
                    const Matrix4x4& modelViewProj) {
  Vector4D q(position, 1.);

  // Transform into clip space
  q = modelViewProj * q;
  double w = q.w;
  q /= w;

  // Shift by (dx, dy).
  q.x += dx;
  q.y += dy;

  // Transform back into model space
  q *= w;
  q = modelViewProj.inv() * q;

  position = q.to3D();
}

StaticScene::SceneObject* Skeleton::get_static_object() { return nullptr; }

DrawStyle* Skeleton::get_draw_style(Joint* joint) const {
  if (scene && joint == scene->selected.object) {
    return selectedStyle;
  }

  if (scene && joint == scene->hovered.object) {
    return hoveredStyle;
  }

  return defaultStyle;
}

void Skeleton::drawJoint(Joint* joint, int& pickID, bool pickMode) {
  Vector3D jointPos = joint->position;
  Vector3D jointRot = joint->rotation;
  Vector3D jointScale = joint->scale;
  Vector3D jointAxis = joint->axis;
  double jointRenderScale = joint->renderScale;
  GLUquadric* q = gluNewQuadric();

  glPushMatrix();

  glTranslated(jointPos.x, jointPos.y, jointPos.z);
  glRotatef(jointRot.x, 1.0f, 0.0f, 0.0f);
  glRotatef(jointRot.y, 0.0f, 1.0f, 0.0f);
  glRotatef(jointRot.z, 0.0f, 0.0f, 1.0f);
  glScalef(jointScale.x, jointScale.y, jointScale.z);

  DrawStyle* style = get_draw_style(joint);
  if (pickMode) {
    newPickElement(pickID, joint);
  } else if (style != defaultStyle) {
    glDisable(GL_LIGHTING);
    style->style_joint();
  } else {
    GLfloat white[4] = {1., 1., 1., 1.};

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);

    glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE);
  }

  glBegin(GL_LINES);
  glVertex3d(0, 0, 0);
  glVertex3d(jointAxis.x, jointAxis.y, jointAxis.z);
  glEnd();
  glTranslated(jointAxis.x, jointAxis.y, jointAxis.z);
  gluSphere(q, jointRenderScale * 0.03, 25, 25);

  if (!pickMode && style != defaultStyle) {
    glEnable(GL_LIGHTING);
    style->style_reset();
  } else {
    glDisable(GL_BLEND);
  }

  for (Joint* child : joint->kids) drawJoint(child, pickID, pickMode);

  glPopMatrix();
  gluDeleteQuadric(q);
}

void Skeleton::setZAxis(Vector3D axis) {
  Vector3D zAxis(0., 0., 1.);
  axis.normalize();
  float rot_angle = acos(dot(axis, zAxis));
  if (fabs(rot_angle) > 10E-6) {
    rot_angle = -rot_angle * 180 / M_PI;
    Vector3D rot_axis = cross(axis, zAxis);
    rot_axis.normalize();
    glRotatef(rot_angle, rot_axis.x, rot_axis.y, rot_axis.z);
  }
}

void drawCube() {
  glBegin(GL_QUADS);

  glVertex3f(1.0, 1.0, -1.0);
  glVertex3f(-1.0, 1.0, -1.0);
  glVertex3f(-1.0, 1.0, 1.0);
  glVertex3f(1.0, 1.0, 1.0);

  glVertex3f(1.0, -1.0, 1.0);
  glVertex3f(-1.0, -1.0, 1.0);
  glVertex3f(-1.0, -1.0, -1.0);
  glVertex3f(1.0, -1.0, -1.0);

  glVertex3f(1.0, 1.0, 1.0);
  glVertex3f(-1.0, 1.0, 1.0);
  glVertex3f(-1.0, -1.0, 1.0);
  glVertex3f(1.0, -1.0, 1.0);

  glVertex3f(1.0, -1.0, -1.0);
  glVertex3f(-1.0, -1.0, -1.0);
  glVertex3f(-1.0, 1.0, -1.0);
  glVertex3f(1.0, 1.0, -1.0);

  glVertex3f(-1.0, 1.0, 1.0);
  glVertex3f(-1.0, 1.0, -1.0);
  glVertex3f(-1.0, -1.0, -1.0);
  glVertex3f(-1.0, -1.0, 1.0);

  glVertex3f(1.0, 1.0, -1.0);
  glVertex3f(1.0, 1.0, 1.0);
  glVertex3f(1.0, -1.0, 1.0);
  glVertex3f(1.0, -1.0, -1.0);

  glEnd();
}

void drawSemisphere(GLdouble radius, GLint slices, GLint stacks) {
  GLUquadric* q = gluNewQuadric();
  glEnable(GL_STENCIL_TEST);

  // Draw half cube
  glStencilFunc(GL_ALWAYS, 1, 0xFF);  // Set any stencil to 1
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilMask(0xFF);    // Write to stencil buffer
  glDepthMask(GL_FALSE);  // Don't write to depth buffer
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE,
              GL_FALSE);           // Don't write to color buffer
  glClear(GL_STENCIL_BUFFER_BIT);  // Clear stencil buffer (0 by default)

  glPushMatrix();
  glTranslatef(0, radius / 2, 0);
  glScalef(radius, radius / 2, radius);
  drawCube();
  glPopMatrix();

  // Draw sphere
  glStencilFunc(GL_EQUAL, 1, 0xFF);  // Pass test if stencil value is 1
  glStencilMask(0x00);               // Don't write anything to stencil buffer
  glDepthMask(GL_TRUE);              // Write to depth buffer
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);  // Write to color buffer
  gluSphere(q, radius, slices, stacks);

  glDisable(GL_STENCIL_TEST);
  gluDeleteQuadric(q);
}

void Skeleton::drawCapsule(Joint* joint) {
  Vector3D jointPos = joint->position;
  Vector3D jointRot = joint->rotation;
  Vector3D jointScale = joint->scale;
  Vector3D jointAxis = joint->axis;
  double axisLength = jointAxis.norm();
  double r = joint->capsuleRadius;
  GLUquadric* q = gluNewQuadric();
  GLfloat white[4] = {1., 1., 1., 1.};
  GLfloat color[4] = {0., 0., 0.25, .25};

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);

  glEnable(GL_BLEND);
  // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBlendFunc(GL_ONE, GL_ONE);

  glPushMatrix();

  glTranslated(jointPos.x, jointPos.y, jointPos.z);
  glRotatef(jointRot.x, 1.0f, 0.0f, 0.0f);
  glRotatef(jointRot.y, 0.0f, 1.0f, 0.0f);
  glRotatef(jointRot.z, 0.0f, 0.0f, 1.0f);
  glScalef(jointScale.x, jointScale.y, jointScale.z);

  glPushMatrix();
  setZAxis(jointAxis);

  glPushMatrix();
  glRotated(-90, 1, 0, 0);
  drawSemisphere(r, 25, 25);
  glPopMatrix();

  gluCylinder(q, r, r, axisLength, 25, 25);

  glPushMatrix();
  glTranslated(0, 0, axisLength);
  glRotated(90, 1, 0, 0);
  drawSemisphere(r, 25, 25);
  glPopMatrix();

  glPopMatrix();

  glTranslated(jointAxis.x, jointAxis.y, jointAxis.z);

  for (Joint* child : joint->kids) drawCapsule(child);

  glPopMatrix();

  glDisable(GL_BLEND);
  gluDeleteQuadric(q);
}

void Skeleton::drawGhost() {
  Vector3D meshPos = mesh->position;
  Vector3D meshRot = mesh->rotation;
  Vector3D meshScale = mesh->scale;
  int dummy;  // not used when pickMode is false, but required

  glPushMatrix();
  glTranslated(meshPos.x, meshPos.y, meshPos.z);
  glRotatef(meshRot.x, 1.0f, 0.0f, 0.0f);
  glRotatef(meshRot.y, 0.0f, 1.0f, 0.0f);
  glRotatef(meshRot.z, 0.0f, 0.0f, 1.0f);
  glScalef(meshScale.x, meshScale.y, meshScale.z);

  drawJoint(root, dummy);
  drawCapsule(root);

  glPopMatrix();
}

void Skeleton::newPickElement(int& pickID, Joint* joint) {
  unsigned char R, G, B;
  IndexToRGB(pickID, R, G, B);
  glColor3ub(R, G, B);
  idToJoint[pickID] = joint;
  pickID++;
}

void Skeleton::draw_pick(int& pickID, bool transformed) {
  if (!isGhosted) return;
  idToJoint.clear();
  Vector3D meshPos = mesh->position;
  Vector3D meshRot = mesh->rotation;
  Vector3D meshScale = mesh->scale;

  glPushMatrix();
  glTranslated(meshPos.x, meshPos.y, meshPos.z);
  glRotatef(meshRot.x, 1.0f, 0.0f, 0.0f);
  glRotatef(meshRot.y, 0.0f, 1.0f, 0.0f);
  glRotatef(meshRot.z, 0.0f, 0.0f, 1.0f);
  glScalef(meshScale.x, meshScale.y, meshScale.z);

  drawJoint(root, pickID, true);

  glPopMatrix();
}

void Skeleton::setSelection(int pickID, Selection& selection) {
  // Set the selection to the joint specified by the given picking ID;
  // these values were generated in Skeleton::draw_pick.
  if (idToJoint.find(pickID) != idToJoint.end()) {
    selection.clear();
    selection.object = idToJoint[pickID];
  }
}

void Skeleton::keyframe(double t) { root->keyframe(t); }

void Skeleton::unkeyframe(double t) { root->unkeyframe(t); }

void Skeleton::reachForTarget(map<Joint*, Vector3D> targets, double time) {
  // TODO (Animation) Task 2B
  // Do several iterations of Jacobian Transpose gradient descent for IK
}

void Skeleton::save(const char* filename) {
  cerr << "Writing skeleton to file " << filename << endl;
  XMLDocument xmlDoc;
  XMLElement* pRoot = xmlDoc.NewElement("Skeleton");
  xmlDoc.InsertFirstChild(pRoot);

  saveJoint(&xmlDoc, pRoot, root);

  XMLError eResult = xmlDoc.SaveFile(filename);
  XMLCheckResult(eResult);
}

void Skeleton::saveJoint(XMLDocument* xmlDoc, XMLElement* parentNode,
                         const Joint* joint) {
  XMLElement* pJoint = xmlDoc->NewElement("Joint");
  parentNode->InsertEndChild(pJoint);

  XMLElement* pElement = xmlDoc->NewElement("Axis");
  pElement->SetAttribute("x", joint->axis.x);
  pElement->SetAttribute("y", joint->axis.y);
  pElement->SetAttribute("z", joint->axis.z);
  pJoint->InsertEndChild(pElement);

  pElement = xmlDoc->NewElement("RenderScale");
  pElement->SetAttribute("val", joint->renderScale);
  pJoint->InsertEndChild(pElement);

  pElement = xmlDoc->NewElement("CapsuleRadius");
  pElement->SetAttribute("val", joint->capsuleRadius);
  pJoint->InsertEndChild(pElement);

  // positions
  pElement = xmlDoc->NewElement("Positions");
  for (const auto& item : joint->positions.knots) {
    XMLElement* pListElement = xmlDoc->NewElement("Knot");
    pListElement->SetAttribute("Time", item.first);
    pListElement->SetAttribute("x", item.second.x);
    pListElement->SetAttribute("y", item.second.y);
    pListElement->SetAttribute("z", item.second.z);

    pElement->InsertEndChild(pListElement);
  }
  pJoint->InsertEndChild(pElement);

  // rotations
  pElement = xmlDoc->NewElement("Rotations");
  for (const auto& item : joint->rotations.knots) {
    XMLElement* pListElement = xmlDoc->NewElement("Knot");
    pListElement->SetAttribute("Time", item.first);
    pListElement->SetAttribute("x", item.second.x);
    pListElement->SetAttribute("y", item.second.y);
    pListElement->SetAttribute("z", item.second.z);

    pElement->InsertEndChild(pListElement);
  }
  pJoint->InsertEndChild(pElement);

  // scales
  pElement = xmlDoc->NewElement("Scales");
  for (const auto& item : joint->scales.knots) {
    XMLElement* pListElement = xmlDoc->NewElement("Knot");
    pListElement->SetAttribute("Time", item.first);
    pListElement->SetAttribute("x", item.second.x);
    pListElement->SetAttribute("y", item.second.y);
    pListElement->SetAttribute("z", item.second.z);

    pElement->InsertEndChild(pListElement);
  }
  pJoint->InsertEndChild(pElement);

  XMLElement* pChildren = xmlDoc->NewElement("Children");
  pJoint->InsertEndChild(pChildren);
  for (Joint* j : joint->kids) {
    saveJoint(xmlDoc, pChildren, j);
  }
}

set<double> Skeleton::load(const char* filename) {
  set<double> knotTimes;
  cerr << "Loading skeleton from file " << filename << endl;
  XMLDocument xmlDoc;
  XMLError eResult = xmlDoc.LoadFile(filename);
  XMLCheckResult(eResult);

  XMLNode* pRoot = xmlDoc.FirstChild();
  if (pRoot == nullptr) {
    cout << "Skeleton file is not a valid XML file." << endl;
    return knotTimes;
  }

  XMLElement* pElement = pRoot->FirstChildElement("Joint");
  if (pElement == nullptr) {
    cout << "Root joint does not exist." << endl;
    return knotTimes;
  }

  // clear skeleton and create a new one
  joints.clear();
  root = loadJoint(pElement, knotTimes);
  root->parent = nullptr;

  return knotTimes;
}

Joint* CMU462::DynamicScene::Skeleton::loadJoint(XMLElement* jointNode,
                                                 set<double>& knotTimes) {
  if (jointNode == nullptr) {
    cout << "Invalid joint node" << endl;
    return nullptr;
  }
  Joint* joint = new Joint(this);

  XMLElement* pElement = jointNode->FirstChildElement("Axis");
  joint->axis.x = pElement->DoubleAttribute("x");
  joint->axis.y = pElement->DoubleAttribute("y");
  joint->axis.z = pElement->DoubleAttribute("z");

  pElement = jointNode->FirstChildElement("RenderScale");
  joint->renderScale = pElement->DoubleAttribute("val");

  pElement = jointNode->FirstChildElement("CapsuleRadius");
  joint->capsuleRadius = pElement->DoubleAttribute("val");

  // positions
  pElement = jointNode->FirstChildElement("Positions");
  XMLElement* pListElement = pElement->FirstChildElement("Knot");
  while (pListElement != nullptr) {
    double time = pListElement->DoubleAttribute("Time");
    double x = pListElement->DoubleAttribute("x");
    double y = pListElement->DoubleAttribute("y");
    double z = pListElement->DoubleAttribute("z");
    Vector3D position(x, y, z);

    joint->positions.setValue(time, position);
    knotTimes.insert(time);
    pListElement = pListElement->NextSiblingElement("Knot");
  }

  // rotations
  pElement = jointNode->FirstChildElement("Rotations");
  pListElement = pElement->FirstChildElement("Knot");
  while (pListElement != nullptr) {
    double time = pListElement->DoubleAttribute("Time");
    double x = pListElement->DoubleAttribute("x");
    double y = pListElement->DoubleAttribute("y");
    double z = pListElement->DoubleAttribute("z");
    Vector3D rotation(x, y, z);

    joint->rotations.setValue(time, rotation);
    knotTimes.insert(time);
    pListElement = pListElement->NextSiblingElement("Knot");
  }

  // scales
  pElement = jointNode->FirstChildElement("Scales");
  pListElement = pElement->FirstChildElement("Knot");
  while (pListElement != nullptr) {
    double time = pListElement->DoubleAttribute("Time");
    double x = pListElement->DoubleAttribute("x");
    double y = pListElement->DoubleAttribute("y");
    double z = pListElement->DoubleAttribute("z");
    Vector3D scale(x, y, z);

    joint->scales.setValue(time, scale);
    knotTimes.insert(time);
    pListElement = pListElement->NextSiblingElement("Knot");
  }

  pElement = jointNode->FirstChildElement("Children");
  XMLElement* pChild = pElement->FirstChildElement("Joint");
  while (pChild != nullptr) {
    Joint* childJoint = loadJoint(pChild, knotTimes);
    childJoint->parent = joint;
    joint->kids.push_back(childJoint);
    pChild = pChild->NextSiblingElement("Joint");
  }

  joints.push_back(joint);

  return joint;
}

Joint* Skeleton::createNewJoint(Joint* parent, Vector3D endPos) {
  Joint* newJoint = new Joint(this);
  newJoint->parent = parent;
  // Get the endPos in joint's local coordinate frame
  Vector4D endPos4d(endPos, 1.);
  Matrix4x4 T = newJoint->getTransformation();
  Matrix4x4 invT = T.inv();
  endPos = (invT * endPos4d).projectTo3D();

  newJoint->axis = endPos;
  double meshScale = mesh->get_bbox().extent.norm();
  newJoint->renderScale = meshScale;
  newJoint->capsuleRadius *= meshScale;
  if (parent != nullptr) parent->kids.push_back(newJoint);
  joints.push_back(newJoint);

  return newJoint;
}

void Skeleton::update(double time) {
  currentTransformation = Matrix4x4::translation(positions.evaluate(time));
}

}  // namespace DynamicScene
}  // namespace CMU462
