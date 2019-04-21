#ifndef CMU462_WIDGETS_H
#define CMU462_WIDGETS_H

#include "scene.h"
#include <vector>
#include <map>

// An XFormWidget is a GUI element for applying spatial
// transformations (rotation, translation, scaling) to
// objects (meshes, lights, cameras, etc.) or individual
// mesh elements (vertices, edges, faces) in a scene.  It
// provides methods for (i) drawing widgets with handles
// for specifying transformations and (ii) conversion of
// user input to 3D transformation matrices, but it is
// NOT responsible for applying the transformation itself.
// For instance, to use an XFormWidget to manipulate a
// mesh element, one might:
//    1. construct a new XFormWidget when the element gets selected,
//    2. pass user IO (e.g., mouse input) to the XFormWidget,
//    3. read back the current transformation matrix from the XFormWidget, and
//    4. apply this matrix to the actual mesh geometry.
// However, the application is responsible for everything but
// step (3)---the XFormWidget will not access the user input
// directly, nor will it apply any transformations to the data.

namespace CMU462 {
namespace DynamicScene {

class XFormWidget : public SceneObject {
 public:
  // Initially, the XFormWidget is not associated with any
  // object (the target object and element will be NULL)
  XFormWidget();

  // Type specifying current mode of operation
  enum class Mode { Translate, Rotate, Scale };

  // Set the object to be transformed
  void setTarget(Selection& target);

  // Set to translate mode
  void setTranslate();

  // Set to rotate mode
  void setRotate();

  // Set to scale mode
  void setScale();

  // Cycle through translate, rotate, and scale modes
  void cycleMode();

  // Return to the previously selected mode
  void restoreLastMode();

  // Set position of initial click on the widget
  void setClickPosition(const Vector2D& position);

  // SceneObject methods
  virtual void set_draw_styles(DrawStyle* defaultStyle, DrawStyle* hoveredStyle,
                               DrawStyle* selectedStyle);
  virtual void draw();
  virtual void drawGhost() { draw(); };
  virtual BBox get_bbox();
  virtual Info getInfo();
  virtual void drag(double x, double y, double dx, double dy,
                    const Matrix4x4& modelViewProj);
  virtual StaticScene::SceneObject* get_static_object();
  virtual void draw_pick(int& pickID, bool transformed = false);
  virtual void setSelection(int pickID, Selection& selection);

  // The target is the object to be transformed by the widget;
  // it is NOT the same as the object currently selected in the
  // viewer, nor the hovered object currently under the cursor.
  // If target.element is not NULL, we assume this transformation
  // is being applied to an individual element; otherwise it is
  // being applied to the whole object target.object.
  Selection target;

  bool getIsTransforming() const;

  // Update the handle geometry to match the currently-selected
  // object.  We don't necessarily want to do this all the time
  // since often we want to provide the user with a consistent
  // reference frame (e.g., during a drag).
  void updateGeometry();

  void onMouseReleased();

  // Toggle object select mode
  void enterObjectMode();
  void exitObjectMode();
  void enterPoseMode();
  void exitPoseMode();
  void enterTransformedMode();
  void exitTransformedMode();

  // Current and most recent modes of operation
  Mode mode, lastMode;

 protected:
  map<int, Selection::Axis> pickIDToAxis;
  vector<Vector3D> axes;
  BBox bounds;
  Vector3D center;

  bool isTransforming = false;

  bool objectMode;
  bool poseMode;
  bool transformedMode;

  void directionalTransform(Vector3D& position, vector<int> transformAxes,
                            Vector3D center, double x, double y, double dx,
                            double dy, const Matrix4x4& modelViewProj);

  void drawHandles() const;
  void drawTranslateHandles() const;
  void drawRotateHandles() const;
  void drawScaleHandles() const;
  void drawCenterHandle() const;

  DrawStyle *defaultStyle, *hoveredStyle, *selectedStyle;

  Vector2D clickPosition;

  vector<vector<GLubyte>> axisColors;
};
}
}

#endif  // CMU462_WIDGETS_H
