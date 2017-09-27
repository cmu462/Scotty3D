#ifndef SKELETON_H
#define SKELETON_H

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
#include "CMU462/tinyxml2.h"
#include "../spline.h"

using namespace std;

namespace CMU462 {
namespace DynamicScene {
class Mesh;
class Joint;

// A Skeleton is a tree of Joints, together with some additional information.
class Skeleton : public SceneObject {
 public:
  Skeleton(Mesh* mesh);

  void set_draw_styles(DrawStyle* defaultStyle, DrawStyle* hoveredStyle,
                       DrawStyle* selectedStyle);

  BBox get_bbox();

  Info getInfo();

  void drag(double x, double y, double dx, double dy,
            const Matrix4x4& modelViewProj);

  StaticScene::SceneObject* get_static_object();

  void draw_pick(int& pickID, bool transformed = false);

  void setSelection(int pickID, Selection& selection);

  /* Save the current state of the skeleton in the splines */
  void keyframe(double t);
  void unkeyframe(double t);

  // Mesh for the skeleton
  Mesh* mesh;

  // The root of the tree.
  Joint* root;

  // In principal, the character could store only its root joint
  // and access its children via a tree traversal.  However, it
  // is often convenient to be able to simply iterate over a list
  // of children.
  vector<Joint*> joints;

  // Transformation of the character at the current time,
  // as computed by the last call to Character::update().
  Matrix4x4 currentTransformation;

  // Creates a new joint with given parent and end position of the new joint.
  // It returns a pointer to the newly created joint.
  Joint* createNewJoint(Joint* parent, Vector3D endPos);

  // Computes the joint transformations and joint center for the
  // specified time, storing these values in Joint::currentTransformation and
  // Joint::currentCenter, respectively.
  void update(double time);

  // draws the character using the specified renderer;
  // note that the layering of joints is determined by
  // a depth-first traversal of the tree (depth-first
  // ordering rather than breadth-first ordering preserves
  // the coherence of limbs, i.e., a whole arm occludes a
  // whole leg)
  void draw(){};

  void drawGhost();

  // The method reachForTarget() optimizes all of the angles in this character
  // in order to bring a source point p on some joint as close as possible to
  // the given
  // target point q.  The source point p is specified in the original coordinate
  // system, i.e.,
  // before applying any joint transformations.  The target point q is specified
  // as a
  // point in world coordinates.  Optimization should be performed by applying
  // gradient
  // descent to the squared norm of the difference between the transformed
  // position of p
  // and the world-space position of q.

  /*
  void reachForTarget( Joint* goalJoint,
        Vector3D targetPoint, // target point q, expressed in the world
  coordinate system (this is the mouse cursor position, so there is no notion of
  "before" and "after" transformation)
        double time ); */
  void reachForTarget(map<Joint*, Vector3D> targets, double t);

  void save(const char* filename);
  // loads the skeleton from a given file and returns a list of times
  // with spline knots
  set<double> load(const char* filename);

 private:
  DrawStyle* get_draw_style(Joint* joint) const;

  // helper function that draws the given joint
  void drawJoint(Joint* joint, int& pickID, bool pickMode = false);

  void setZAxis(Vector3D axis);

  // helper function that draws the skinning capsule around the given joint
  void drawCapsule(Joint* joint);

  DrawStyle *defaultStyle, *hoveredStyle, *selectedStyle;

  // map from picking IDs to joints, generated during draw_pick
  // and used by setSelection
  std::map<int, Joint*> idToJoint;

  // Assigns the next consecutive pickID to the given element, and
  // sets the GL color accordingly.  Also increments pickID.
  void newPickElement(int& pickID, Joint* joint);

  // helper functions for loading and saving joint data
  void saveJoint(tinyxml2::XMLDocument* xmlDoc,
                 tinyxml2::XMLElement* parentNode, const Joint* joint);
  Joint* loadJoint(tinyxml2::XMLElement* jointNode, set<double>& knotTimes);
};
}  // namespace DynamicScene
}  // namespace CMU462

#endif  // SKELETON_H
