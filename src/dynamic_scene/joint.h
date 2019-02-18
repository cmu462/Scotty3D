#ifndef JOINT_H
#define JOINT_H

/*
 * Character class and Joint Class.
 * Written by Bryce Summers on October 29, 2015.
 *
 * Purpose : These classes represent a rigged character that can be animated.
 *
 */

#include <vector>
#include "CMU462/vector3D.h"
#include "CMU462/matrix4x4.h"
#include "../spline.h"
#include "scene.h"
#include "draw_style.h"

using namespace std;

namespace CMU462 {
namespace DynamicScene {
class Skeleton;

class Joint : public SceneObject {
 public:
  Joint(Skeleton* skeleton);

  void set_draw_styles(DrawStyle* defaultStyle, DrawStyle* hoveredStyle,
                       DrawStyle* selectedStyle){};

  BBox get_bbox();

  Info getInfo();

  void drag(double x, double y, double dx, double dy,
            const Matrix4x4& modelViewProj);

  StaticScene::SceneObject* get_static_object();

  void draw_pick(int& _pickID, bool transformed = false){};

  void draw(){};

  void drawGhost(){};

  void setSelection(int _pickID, Selection& selection){};

  void keyframe(double t);
  void unkeyframe(double t);

  void removeJoint(Scene* scene);

  void getAxes(vector<Vector3D>& axes);

  // Returns the transformation up to the base of this joint
  // (rotations of this joint is not applied)
  Matrix4x4 getTransformation();

  // Returns the transformation without any rotations applied
  Matrix4x4 getBindTransformation();

  // Each joint has some number of children (possibly zero).
  vector<Joint*> kids;

  // Vector pointing from the parent joint toward the children
  // in the rest configuration of the joint.
  Vector3D axis;

  // Gradient of IK energy with respect to this joint angle, which will
  // be used to update the joint configurations.  This value is updated
  // by calls to Character::reachForTarget().
  Vector3D ikAngleGradient;

  // Returns the joint angle.  If the joint motion is determined by keyframe
  // animation, this
  // angle will be the interpolated angle at the given time; if the joint motion
  // is determined
  // by dynamics, this angle will simply be the most recently computed angle.
  Vector3D getAngle(double time);

  // Sets the joint angle.  If the joint motion is determined by keyframe
  // animation, this method
  // sets the angle for the spline at the specified time; if it is determined by
  // dynamics, this
  // method simply sets the dynamical variable at the current time to the
  // specified value.
  void setAngle(double time, Vector3D value);

  // Removes any keyframe corresponding to the specified time.
  bool removeAngle(double time);

  // Computes the gradient of IK energy for this joint and, recursively,
  // for all of its children, storing the result in Joint::ikAngleGradient.
  void calculateAngleGradient(Joint* goalJoint, Vector3D ptilde);

  // Get base position of the joint in world coordinate frame
  Vector3D getBasePosInWorld();

  // Get end position of the joint in world coordinate frame
  Vector3D getEndPosInWorld();

  // Index into the "joints" array of this Joint's Character.
  // This value is used exclusively for OpenGL picking; it
  // should not be used (or needed) by any other routine.
  int pickID;

  // Pointer to skeleton
  Skeleton* skeleton;

  // Pointer to parent
  Joint* parent;

  // Pointer to other joint created with joint symmetry, if symmetry is on
  Joint* mirror = nullptr;

  // Radius of the capsule around the joint for skinning
  double capsuleRadius;

  // Rendering scale of the joint
  double renderScale;
};
}  // namespace DynamicScene
}  // namespace CMU462

#endif  // JOINT_H
