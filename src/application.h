#ifndef CMU462_APPLICATION_H
#define CMU462_APPLICATION_H

// STL
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>

// libCMU462
#include "CMU462/CMU462.h"
#include "CMU462/renderer.h"
#include "CMU462/osdtext.h"

// COLLADA
#include "collada/collada.h"
#include "collada/light_info.h"
#include "collada/sphere_info.h"
#include "collada/polymesh_info.h"
#include "collada/material_info.h"

// MeshEdit
#include "dynamic_scene/scene.h"
#include "dynamic_scene/widgets.h"
#include "halfEdgeMesh.h"
#include "meshEdit.h"

// PathTracer
#include "static_scene/scene.h"
#include "pathtracer.h"
#include "image.h"

// Animator
#include "timeline.h"

// Shared modules
#include "camera.h"

using namespace std;

namespace CMU462 {

struct AppConfig {
  AppConfig() {
    pathtracer_ns_aa = 1;
    pathtracer_max_ray_depth = 1;
    pathtracer_ns_area_light = 4;

    pathtracer_ns_diff = 1;
    pathtracer_ns_glsy = 1;
    pathtracer_ns_refr = 1;

    pathtracer_num_threads = 1;
    pathtracer_envmap = NULL;
    pathtracer_result_path = "";
  }

  size_t pathtracer_ns_aa;
  size_t pathtracer_max_ray_depth;
  size_t pathtracer_ns_area_light;
  size_t pathtracer_ns_diff;
  size_t pathtracer_ns_glsy;
  size_t pathtracer_ns_refr;
  size_t pathtracer_num_threads;
  HDRImageBuffer* pathtracer_envmap;
  std::string pathtracer_result_path;
  size_t pathtracer_result_width = 800;
  size_t pathtracer_result_height = 600;
};

class Application : public Renderer {
 public:
  Application(AppConfig config);

  ~Application();

  void init();
  void render();
  void resize(size_t w, size_t h);

  std::string name();
  std::string info();

  void cursor_event(float x, float y);
  void scroll_event(float offset_x, float offset_y);
  void mouse_event(int key, int event, unsigned char mods);
  void keyboard_event(int key, int event, unsigned char mods);
  void char_event(unsigned int codepoint);

  void load(Collada::SceneInfo* sceneInfo);
  void writeScene(const char* filename);
  void loadScene(const char* filename);

  void writeSkeleton(const char* filename, const DynamicScene::Scene* scene);
  void loadSkeleton(const char* filename, DynamicScene::Scene* scene);

  void render_scene(std::string saveFileLocation);

  // Avoids spinning up an OpenGL context during initialization.
  // This useful because it avoid issues with OpenGL when SSH'ed, so users
  // can (for example) test their pathtracer output without requiring OpenGL.
  // Note: The ONLY operation that is supported in headless mode is pathtracing.
  //       the application will most likely crash if you try to call anything else.
  bool init_headless = false;

 private:
  // Mode determines which type of data is visualized/
  // which mode we're currently in (e.g., modeling vs. rendering vs. animation)
  enum Mode { MODEL_MODE, RENDER_MODE, ANIMATE_MODE, VISUALIZE_MODE };
  Mode mode;

  // Action determines which action will be taken when
  // the user clicks/drags/etc.
  enum class Action {
    Navigate,
    Edit,
    Bevel,
    CreateJoint,
    BoneRadius,
    IK,
    Wave,
    Object,
    Pose,
    Raytrace_Video,
    Rasterize_Video
  };
  Action action;

  bool noGUI;

  enum class Integrator { Forward_Euler, Symplectic_Euler };
  Integrator integrator;

  map<DynamicScene::Joint*, Vector3D> ikTargets;

  void switch_modes(unsigned int key);
  void to_model_mode();
  void to_render_mode();
  void to_animate_mode();
  void to_visualize_mode();
  void to_navigate_action();
  void toggle_bevel_action();
  void toggle_create_joint_action();
  void to_wave_action();
  void to_object_action();
  void to_pose_action();
  void cycle_edit_action();
  void set_up_pathtracer();
  void raytrace_video();
  void rasterize_video();

  DynamicScene::Scene* scene;
  PathTracer* pathtracer;

  // View Frustrum Variables.
  // On resize, the aspect ratio is changed. On reset_camera, the position and
  // orientation are reset but NOT the aspect ratio.
  Camera camera;
  Camera canonicalCamera;

  size_t screenW;
  size_t screenH;

  double timestep;
  double damping_factor;

  // Length of diagonal of bounding box for the mesh.
  // Guranteed to not have the camera occlude with the mes.
  double canonical_view_distance;

  // Rate of translation on scrolling.
  double scroll_rate;

  /*
    Called whenever the camera fov or screenW/screenH changes.
  */
  void set_projection_matrix();

  /**
   * Fills the DrawStyle structs.
   */
  void initialize_style();

  /**
   * Update draw styles properly given the current view distance.
   */
  void update_style();

  /**
   * Reads and combines the current modelview and projection matrices.
   */
  Matrix4x4 get_world_to_3DH();

  // Initialization functions to get the opengl cooking with oil.
  void init_camera(Collada::CameraInfo& camera, const Matrix4x4& transform);
  DynamicScene::SceneLight* init_light(Collada::LightInfo& light,
                                       const Matrix4x4& transform);
  DynamicScene::SceneObject* init_sphere(Collada::SphereInfo& polymesh,
                                         const Matrix4x4& transform);
  DynamicScene::SceneObject* init_polymesh(Collada::PolymeshInfo& polymesh,
                                           const Matrix4x4& transform);
  void init_material(Collada::MaterialInfo& material);

  void set_scroll_rate();

  // Resets the camera to the canonical initial view position.
  void reset_camera();

  // Rendering functions.
  void update_gl_camera();

  // style for elements that are neither hovered nor selected
  DynamicScene::DrawStyle defaultStyle;
  DynamicScene::DrawStyle hoverStyle;
  DynamicScene::DrawStyle selectStyle;

  // Internal event system //

  float mouseX, mouseY;
  enum e_mouse_button {
    LEFT = MOUSE_LEFT,
    RIGHT = MOUSE_RIGHT,
    MIDDLE = MOUSE_MIDDLE
  };

  bool leftDown;
  bool rightDown;
  bool middleDown;

  // Only draw the pick buffer so often
  // as an optimization.
  int pickDrawCountdown = 0;
  int pickDrawInterval = 5;

  // Event handling //
  void mouse_pressed(e_mouse_button b);   // Mouse pressed.
  void mouse_released(e_mouse_button b);  // Mouse Released.
  void mouse1_dragged(float x, float y);  // Left Mouse Dragged.
  void mouse2_dragged(float x, float y);  // Right Mouse Dragged.

  /**
   * If there is current selection and it's draggable, apply its drag method.
   */
  void dragSelection(float x, float y, float dx, float dy,
                     const Matrix4x4& modelViewProj);

  /**
   * If the cursor is hovering over something, mark it as selected.
   */
  void selectHovered();

  void updateWidgets();
  void setupElementTransformWidget();

  // OSD text manager //
  OSDText textManager;
  Color text_color;
  vector<int> messages;

  // Coordinate System //
  bool show_coordinates;
  void draw_coordinates();

  // 3D Transformation //
  void setXFormWidget();

  // HUD //
  bool show_hud;
  void draw_hud();
  void draw_action();
  inline void draw_string(float x, float y, string str, size_t size,
                          const Color& c);

  bool lastEventWasModKey;

  // Animator timeline
  Timeline timeline;
  bool draggingTimeline;
  void enter_2D_GL_draw_mode();
  void exit_2D_GL_draw_mode();

  // Skeleton creation
  DynamicScene::Joint* clickedJoint;  // joint that is selected

  // Intersects mouse position x, y in screen coordinates with a plane
  // going through the origin, and returns the intersecting position
  Vector3D getMouseProjection(double dist=std::numeric_limits<double>::infinity());

  // File read/write --- since we don't have a proper implementation
  // of undo/redo, we give the user 10 "buffers" they can write into
  // and load out of.  This way they can save their work before they
  // try a dangerous edit, and restore it if they're unhappy with the
  // result.  These buffers actually just write files to disk (with
  // labels 0-9) and load them back in as needed.  To provide some
  // protection against accidentally overwriting important data,
  // the user has to press a sequence of keys to load and/or write,
  // namely, they have to press 'w' and then a number 0-9 to write,
  // or 'l' and a number 0-9 to load.  If the file already exists
  // during write, it will be overwritten.  If the file does not
  // exist during load, nothing will happen.
  bool readyWrite, readyLoad;
  void queueWrite();
  void queueLoad();
  void executeFileOp(int codepoint);

  bool isGhosted;
  void setGhosted(bool isGhosted);
  void toggleGhosted();

  bool useCapsuleRadius;

  GLuint backface_color_tex;
  GLuint backface_depth_tex;
  GLuint backface_fbo;
  GLuint frntface_color_tex;
  GLuint frntface_depth_tex;
  GLuint frntface_fbo;

  bool symmetryEnabled = false;
  enum Axis { X = 'X', Y = 'Y', Z = 'Z' };
  Axis symmetryAxis = Axis::X;
};  // class Application

}  // namespace CMU462

#endif  // CMU462_APPLICATION_H
