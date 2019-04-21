#ifndef CMU462_DYNAMICSCENE_SCENE_H
#define CMU462_DYNAMICSCENE_SCENE_H

#include <string>
#include <vector>
#include <set>
#include <iostream>

#include "CMU462/CMU462.h"
#include "CMU462/color.h"

#include "GL/glew.h"

#include "draw_style.h"

#include "../bbox.h"
#include "../ray.h"
#include "../static_scene/scene.h"
#include "../halfEdgeMesh.h"
#include "../spline.h"
#include "../timeline.h"

namespace CMU462 {
namespace DynamicScene {

// Forward declarations
class Scene;
class Selection;
class XFormWidget;

/**
 * Interface that all physical objects in the scene conform to.
 * Note that this doesn't include properties like material that may be treated
 * as separate entities in a COLLADA file, or lights, which are treated
 * specially.
 */
class SceneObject {
 public:
  SceneObject()
      : scene(NULL), isVisible(true), isGhosted(false), isPickable(true) {}

  /**
   * Passes in logic for how to render the object in OpenGL.
   */
  virtual void set_draw_styles(DrawStyle *defaultStyle, DrawStyle *hoveredStyle,
                               DrawStyle *selectedStyle) = 0;

  /**
   * Renders the object in OpenGL, assuming that the camera and projection
   * matrices have already been set up.
   */
  virtual void draw() = 0;

  virtual void draw_pretty() { draw(); }

  /**
   * Renders a semi-transparent version of the object, for situations
   * where the object is inactive or something inside the object needs
   * to be selected.  Note that this method does not need to be defined
   * for all scene objects (default is to draw nothing).  Assumes that
   * the camera and projection matrices have already been set up.
   */
  virtual void drawGhost(){};

  /**
   * Given a transformation matrix from local to space to world space, returns
   * a bounding box of the object in world space. Note that this doesn't have
   * to be the smallest possible bbox, in case that's difficult to compute.
   */
  virtual BBox get_bbox() = 0;

  /**
   * Returns info about the current selection (null if this object doesn't have
   * a selection), for use in drawHUD.
   */
  virtual Info getInfo() = 0;

  /**
   * Perform some action associated with dragging the cursor a distance (dx,dy),
   * possibly making use of the current model-view-projection matrix.
   */
  virtual void drag(double x, double y, double dx, double dy,
                    const Matrix4x4 &modelViewProj) = 0;

  /**
   * Converts this object to an immutable, raytracer-friendly form. Passes in a
   * local-space-to-world-space transformation matrix, because the raytracer
   * expects all the objects to be
   */
  virtual StaticScene::SceneObject *get_static_object() = 0;
  /**
   * Does the same thing as get_static_object, but applies the object's
   * transformations first.
   */
  virtual StaticScene::SceneObject *get_transformed_static_object(double t) {
    return get_static_object();
  }

  /**
   * Rather than drawing the object geometry for display, this method draws the
   * object with unique colors that can be used to determine which object was
   * selected or "picked" by the cursor.  The parameter pickID is the lowest
   * consecutive integer that has so far not been used by any other object as
   * a picking ID.  (Draw colors are then derived from these IDs.)  This data
   * will be used by Scene::getHoveredObject to make the final determination
   * of which object (and possibly element within that object) was picked.
   */
  virtual void draw_pick(int &pickID, bool transformed = false) = 0;

  /** Assigns attributes of the selection based on the ID of the
   * object that was picked.  Can assume that pickID was one of
   * the IDs generated during this object's call to draw_pick().
   */
  virtual void setSelection(int pickID, Selection &selection) = 0;

  virtual Matrix4x4 getTransformation();

  virtual Matrix4x4 getRotation();

  /**
   * Pointer to the parent scene containing this object.
   */
  Scene *scene;

  /* World-space position, rotation, scale */
  Vector3D position;
  Vector3D rotation;
  Vector3D scale;

  /* Spline interpolation data / functions */
  Spline<Vector3D> positions;
  Spline<Vector3D> rotations;
  Spline<Vector3D> scales;

  /**
   * Is this object drawn in the scene?
   */
  bool isVisible;

  /**
   * Is this object drawn as semi-transparent?
   */
  bool isGhosted;

  /**
   * Is this object pickable right now?
   */
  bool isPickable;
};

// A Selection stores information about any object or widget that is
// selected in the scene, which could include a mesh, a light, a
// camera, or any piece of an object, such as a vertex in a mesh or
// a rotation handle on a camera.
class Selection {
 public:
  // Types used for scene elements that have well-
  // defined axes (e.g., transformation widgets)
  enum class Axis { X, Y, Z, Center, None };

  Selection() { clear(); }

  void clear() {
    object = nullptr;
    element = nullptr;
    coordinates = Vector3D(0., 0., 0.);
    axis = Axis::None;
  }

  bool operator==(const Selection &s) const {
    return object == s.object && element == s.element && axis == s.axis &&
           coordinates.x == s.coordinates.x &&
           coordinates.y == s.coordinates.y && coordinates.z == s.coordinates.z;
  }

  bool operator!=(const Selection &s) const { return !(*this == s); }

  SceneObject *object;       // the selected object
  HalfedgeElement *element;  // unused unless object is a mesh
  Vector3D coordinates;      // for optionally selecting a single point
  Axis axis;                 // for optionally selecting an axis
};

/**
 * A light.
 */
class SceneLight {
 public:
  virtual StaticScene::SceneLight *get_static_light() const = 0;
};

/**
 * The scene that meshEdit generates and works with.
 */
class Scene {
 public:
  Scene(std::vector<SceneObject *> _objects, std::vector<SceneLight *> _lights);
  ~Scene();

  static Scene deep_copy(Scene s);
  void apply_transforms(double t);

  /**
   * Attempts to add object o to the scene, returning
   * false if it is already in the scene.
   */
  bool addObject(SceneObject *o);

  /**
   * Attempts to remove object o from the scene, returning
   * false if it is not already in the scene.
   */
  bool removeObject(SceneObject *o);

  /**
   * Passes instructions to every object in the scene for how to render
   * themselves in OpenGL.
   */
  void set_draw_styles(DrawStyle *defaultStyle, DrawStyle *hoveredStyle,
                       DrawStyle *selectedStyle);
  /**
   * Renders the scene in OpenGL, assuming the camera and projection
   * transformations have been applied elsewhere.
   */
  void render_in_opengl();

  /**
   * Renders the scene at the given time in OpenGL, according to the
   * splines specified in the animator.
   */
  void render_splines_at(double time, bool pretty, bool useCapsuleRadius, bool depth_only = false);

  /**
   * Draws the actual curves corresponding to a spline.
   */
  void draw_spline_curves(Timeline &timeline);

  /**
   * Gets a bounding box for the entire scene in world space coordinates.
   * May not be the tightest possible.
   */
  BBox get_bbox();

  /**
   * Finds the object pointed to by the given (x, y) point.
   * x and y are from -1 to 1, NOT screenW to screenH.
   * Note that hoverIdx (and therefore has_hover) is automatically updated every
   * time this function is called.
   */
  void getHoveredObject(const Vector2D &p, bool getElement = true,
                        bool transformed = true);

  /**
   * Returns true iff there is a hovered feature in the scene.
   */
  bool has_hover();

  /**
   * Returns true iff there is a selected feature in the scene.
   */
  bool has_selection();

  bool has_bevel();

  void bevel_selected_element();
  void update_bevel_amount(float dx, float dy);

  /**
   * Returns information about the given selection, or nullptr if there is none.
   * Note that this object is still owned by the Scene, so it is invalidated on
   * selection updates, scene changes, and scene destruction.
   */
  Info getSelectionInfo();

  void collapse_selected_element();
  void flip_selected_edge();
  void split_selected_edge();
  void erase_selected_element();
  void erase_selected_joint();

  void upsample_selected_mesh();
  void downsample_selected_mesh();
  void resample_selected_mesh();

  void selectNextHalfedge();
  void selectTwinHalfedge();
  void selectHalfedge();

  void triangulateSelection();
  void subdivideSelection(bool useCatmullClark = false);

  /**
   * Builds a static scene that's equivalent to the current scene and is easier
   * to use in raytracing, but doesn't allow modifications.
   */
  StaticScene::Scene *get_static_scene();
  /**
   * Does the same thing as get_static_scene, but applies all objects'
   * transformations.
   */
  StaticScene::Scene *get_transformed_static_scene(double t);

  /* Keep track of which elements of the scene (if any) are currently
   * under the cursor, selected, or being edited. */
  Selection hovered;
  Selection selected;
  Selection edited;

  // This widget is a scene object that is used to manipulate
  // mesh elements (translate, rotate, scale, etc.)
  DynamicScene::XFormWidget *elementTransform;

  /** Clears any selections, hovers, targets, or any other notion
   * of objects/elements being selected in the scene.  (This method
   * is useful after changes to the connectivity, which may invalidate
   * selected elements.)
   */
  void clearSelections();

  std::set<SceneObject *> objects;
  std::set<SceneLight *> lights;

 private:
  Info selectionInfo;

  DrawStyle *defaultStyle, *hoveredStyle, *selectedStyle;

  /**
   * Gets the selected object from the scene, returning nullptr if no object is
   * selected.
   */
  SceneObject *get_world_to_3DH();
};

// Mapping between integer and 8-bit RGB values (used for picking)
static inline void IndexToRGB(int i, unsigned char &R, unsigned char &G,
                              unsigned char &B) {
  R = (i & 0x000000FF) >> 0;
  G = (i & 0x0000FF00) >> 8;
  B = (i & 0x00FF0000) >> 16;
}

// Mapping between 8-bit RGB values and integer (used for picking)
static inline int RGBToIndex(unsigned char R, unsigned char G,
                             unsigned char B) {
  return R + G * 256 + 256 * 256 * B;
}

}  // namespace DynamicScene
}  // namespace CMU462

#endif  // CMU462_DYNAMICSCENE_DYNAMICSCENE_H
