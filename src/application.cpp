#include "application.h"
#include "dynamic_scene/ambient_light.h"
#include "dynamic_scene/environment_light.h"
#include "dynamic_scene/directional_light.h"
#include "dynamic_scene/area_light.h"
#include "dynamic_scene/point_light.h"
#include "dynamic_scene/spot_light.h"
#include "dynamic_scene/sphere.h"
#include "dynamic_scene/mesh.h"
#include "dynamic_scene/widgets.h"
#include "dynamic_scene/skeleton.h"
#include "dynamic_scene/joint.h"

#include "CMU462/lodepng.h"

#include "GLFW/glfw3.h"

#include <functional>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <thread>
using namespace std;

using Collada::CameraInfo;
using Collada::LightInfo;
using Collada::MaterialInfo;
using Collada::PolymeshInfo;
using Collada::SceneInfo;
using Collada::SphereInfo;

namespace CMU462 {

Application::Application(AppConfig config) {
  scene = nullptr;

  pathtracer =
      new PathTracer(config.pathtracer_ns_aa, config.pathtracer_max_ray_depth,
                     config.pathtracer_ns_area_light, config.pathtracer_ns_diff,
                     config.pathtracer_ns_glsy, config.pathtracer_ns_refr,
                     config.pathtracer_num_threads, config.pathtracer_envmap);

  timestep = 0.1;
  damping_factor = 0.0;

  readyWrite = false;
  readyLoad = false;
  useCapsuleRadius = true;

  screenW = config.pathtracer_result_width;
  screenH = config.pathtracer_result_height;
}

Application::~Application() {
  if (pathtracer != nullptr) delete pathtracer;
  if (scene != nullptr) delete scene;
}

void Application::init() {
  if (scene != nullptr) {
    delete scene;
    scene = nullptr;
  }

  // Setup all the basic internal state to default values,
  // as well as some basic OpenGL state (like depth testing
  // and lighting).

  // Set the integer bit vector representing which keys are down.
  leftDown = false;
  rightDown = false;
  middleDown = false;

  show_coordinates = true;
  show_hud = true;

  // When rendering "headless" we don't want to spin up an OpenGL context,
  // so that users can SSH into a machine to test their pathtracer.
  // Headless mode is enabled when the -w flag is passed.
  if (!init_headless) {
      // Lighting needs to be explicitly enabled.
      glEnable(GL_LIGHTING);

      // Enable anti-aliasing and circular points.
      glEnable(GL_LINE_SMOOTH);
      // glEnable( GL_POLYGON_SMOOTH ); // XXX causes cracks!
      glEnable(GL_POINT_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      // glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
      glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

      // initialize fbos
      glGenFramebuffers(1, &backface_fbo);
      glGenFramebuffers(1, &frntface_fbo);
      glGenTextures(1, &backface_color_tex);
      glGenTextures(1, &backface_depth_tex);
      glGenTextures(1, &frntface_color_tex);
      glGenTextures(1, &frntface_depth_tex);

      textManager.init(use_hdpi);
      text_color = Color(1.0, 1.0, 1.0);
  }


  // Initialize styles (colors, line widths, etc.) that will be used
  // to draw different types of mesh elements in various situations.
  initialize_style();

  mode = MODEL_MODE;
  action = Action::Navigate;
  scene = nullptr;

  // Make a dummy camera so resize() doesn't crash before the scene has been
  // loaded.
  // NOTE: there's a chicken-and-egg problem here, because load()
  // requires init, and init requires init_camera (which is only called by
  // load()).
  if (screenW == 0 || screenH == 0)
    screenW = screenH = 600;
  CameraInfo cameraInfo;
  cameraInfo.hFov = 20;
  cameraInfo.vFov = 28;
  cameraInfo.nClip = 0.1;
  cameraInfo.fClip = 100;
  camera.configure(cameraInfo, screenW, screenH);
  canonicalCamera.configure(cameraInfo, screenW, screenH);

  // Now initialize the timeline
  timeline.setMaxFrame(300);
  timeline.action_rewind();
  timeline.resize(screenW, 64);
  timeline.move(0, screenH - 64);
  timeline.markTime(0);
}

void Application::initialize_style() {
  // Colors.
  defaultStyle.halfedgeColor = Color(0.3, 0.3, 0.3, 1.0);
  hoverStyle.halfedgeColor = Color(0.8, 0.8, 1.0, 1.0);
  selectStyle.halfedgeColor = Color(0.4, 0.4, 0.8, 1.0);

  defaultStyle.faceColor = Color(0.2, 0.2, 0.2, 1.0);
  hoverStyle.faceColor = Color(0.8, 0.8, 1.0, 1.0);
  selectStyle.faceColor = Color(0.4, 0.4, 0.8, 1.0);

  defaultStyle.edgeColor = Color(0.3, 0.3, 0.3, 1.0);
  hoverStyle.edgeColor = Color(0.8, 0.8, 1.0, 1.0);
  selectStyle.edgeColor = Color(0.4, 0.4, 0.8, 1.0);

  defaultStyle.vertexColor = Color(0.3, 0.3, 0.3, 1.0);
  hoverStyle.vertexColor = Color(0.8, 0.8, 1.0, 1.0);
  selectStyle.vertexColor = Color(0.4, 0.4, 0.8, 1.0);

  defaultStyle.jointColor = Color(0.3, 0.3, 0.3, 1.0);
  hoverStyle.jointColor = Color(0.8, 0.8, 1.0, 1.0);
  selectStyle.jointColor = Color(0.4, 0.4, 0.8, 1.0);

  // Primitive sizes.
  defaultStyle.strokeWidth = 1.0;
  hoverStyle.strokeWidth = 4.0;
  selectStyle.strokeWidth = 4.0;

  defaultStyle.vertexRadius = 8.0;
  hoverStyle.vertexRadius = 16.0;
  selectStyle.vertexRadius = 16.0;
}

void Application::enter_2D_GL_draw_mode() {
  int screen_w = screenW;
  int screen_h = screenH;
  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(0, 0, screen_w, screen_h);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, screen_w, screen_h, 0, 0, 1);  // Y flipped !
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(0, 0, -1);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
}

void Application::exit_2D_GL_draw_mode() {
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
}

void Application::update_style() {
  float view_distance = (camera.position() - camera.view_point()).norm();
  float scale_factor = canonical_view_distance / view_distance;

  hoverStyle.strokeWidth = 2.0 * scale_factor;
  selectStyle.strokeWidth = 2.0 * scale_factor;

  hoverStyle.vertexRadius = 8.0 * scale_factor;
  selectStyle.vertexRadius = 8.0 * scale_factor;
}

void Application::render() {
  // Update the hovered element using the pick buffer once very n iterations.
  // We do this here rather than on mouse move, because some platforms generate
  // an excessive number of mouse move events which incurs a performance hit.
  if(pickDrawCountdown < 0) {
    Vector2D p(mouseX, screenH - mouseY);
    scene->getHoveredObject(p);
    pickDrawCountdown += pickDrawInterval;
  } else {
    pickDrawCountdown--;
  }

  glClearColor(0., 0., 0., 0.);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  update_gl_camera();

  if (mode == ANIMATE_MODE && action == Action::CreateJoint) {
    Vector3D pos = getMouseProjection();
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    GLUquadric *q = gluNewQuadric();
    glColor3f(0.3, 0.0, 0.0);
    gluSphere(q, 0.1, 25, 25);
    glPopMatrix();
    gluDeleteQuadric(q);

    if(symmetryEnabled) {
      Vector3D mpos = pos;
      switch(symmetryAxis) {
        case X: mpos.x *= -1; break;
        case Y: mpos.y *= -1; break;
        case Z: mpos.z *= -1; break;
      }

      glPushMatrix();
      glTranslated(mpos.x, mpos.y, mpos.z);
      GLUquadric *q = gluNewQuadric();
      glColor3f(0.3, 0.0, 0.0);
      gluSphere(q, 0.1, 25, 25);
      glPopMatrix();
      gluDeleteQuadric(q);
    }
  }

  // Need to clear depth buffers after calling getMouseProjection
  glBindFramebuffer(GL_FRAMEBUFFER, backface_fbo);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindFramebuffer(GL_FRAMEBUFFER, frntface_fbo);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // Rebind framebuffer to active
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  std::function<void(bool)> render_fn = [](bool b){};

  switch (mode) {
    case MODEL_MODE:
      render_fn = [this](bool depth){
        if (!depth && show_coordinates) draw_coordinates();

        scene->render_in_opengl();

        if (!depth && show_hud) draw_hud();
      };

      break;
    case VISUALIZE_MODE:
      if (show_coordinates) draw_coordinates();
    case RENDER_MODE:
      pathtracer->update_screen();
      break;
    case ANIMATE_MODE:
      if (timeline.isCurrentlyPlaying()) {
        for (auto o : scene->objects) {
          DynamicScene::Mesh *mesh = dynamic_cast<DynamicScene::Mesh *>(o);
          if (mesh != nullptr) {
            for (int i = 0; i < 2; i++) {
              switch (integrator) {
                case Integrator::Forward_Euler:
                  mesh->forward_euler(timestep, damping_factor);
                  break;
                case Integrator::Symplectic_Euler:
                  mesh->symplectic_euler(timestep, damping_factor);
                  break;
              }
            }
          }
        }
      }
      if (action == Action::Raytrace_Video) {
        raytrace_video();
        return;
      }

      if (action == Action::IK && rightDown) {
        if (scene->has_selection()) {
          if (scene->selected.object->getInfo()[0] == string("JOINT")) {
            DynamicScene::Joint *j =
                (DynamicScene::Joint *)scene->selected.object;
            DynamicScene::Skeleton *skel = j->skeleton;
            if (ikTargets.find(j) != ikTargets.end()) {
              ikTargets.erase(j);
            }
            double dist = (j->getEndPosInWorld() - camera.position()).norm();
            ikTargets.emplace(j, getMouseProjection(dist));
            skel->reachForTarget(ikTargets, timeline.getCurrentFrame());
            timeline.markTime(timeline.getCurrentFrame());
          }
        }
      }

      timeline.step();

      render_fn = [this](bool depth){
        if (!depth && show_coordinates && !timeline.isCurrentlyPlaying())
          draw_coordinates();
        if (!depth && !timeline.isCurrentlyPlaying())
          scene->draw_spline_curves(timeline);

        scene->render_splines_at(timeline.getCurrentFrame(),
                               timeline.isCurrentlyPlaying(), useCapsuleRadius, depth);

        if(!depth && !noGUI) {
          enter_2D_GL_draw_mode();
          timeline.mouse_over(mouseX, mouseY);
          timeline.draw();
          exit_2D_GL_draw_mode();
        }
      };

      if (action == Action::Rasterize_Video) {
        rasterize_video();
      }
      break;
  }

  { // scope for using statements
    using CMU462::DynamicScene::Mesh;
    using CMU462::DynamicScene::RenderMask;

    //glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    Mesh::flip_normals = true;
    Mesh::global_render_mask = RenderMask::FACE;

    glBindFramebuffer(GL_FRAMEBUFFER, backface_fbo);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);
    render_fn(true);

    Mesh::flip_normals = false;

    glBindFramebuffer(GL_FRAMEBUFFER, frntface_fbo);
    glFrontFace(GL_CCW);
    render_fn(true);

    glDisable(GL_CULL_FACE);
    Mesh::global_render_mask = RenderMask::ALL;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    render_fn(false);
  }

  draw_action();
}

void Application::update_gl_camera() {
  // Call resize() every time we draw, since it doesn't seem
  // to get called by the Viewer upon initial window creation
  // (this should probably be fixed!).
  GLint view[4];
  glGetIntegerv(GL_VIEWPORT, view);
  if (view[2] != screenW || view[3] != screenH) {
    resize(view[2], view[3]);
  }

  // Control the camera to look at the mesh.
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  const Vector3D &c = camera.position();
  const Vector3D &r = camera.view_point();
  const Vector3D &u = camera.up_dir();

  gluLookAt(c.x, c.y, c.z, r.x, r.y, r.z, u.x, u.y, u.z);
}

void Application::resize(size_t w, size_t h) {
  screenW = w;
  screenH = h;
  camera.set_screen_size(w, h);
  set_projection_matrix();
  if (mode != MODEL_MODE) {
    pathtracer->set_frame_size(w, h);
  }
  timeline.resize(w, 64);
  timeline.move(0, h - 64);

  if (init_headless)
      return;

  textManager.resize(w, h);

  auto set_params = [](bool depth) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if(depth) {
      glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }
  };

  // update backface fbo texture
  glBindFramebuffer(GL_FRAMEBUFFER, backface_fbo);
  glBindTexture(GL_TEXTURE_2D, backface_color_tex);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, screenW, screenH, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  set_params(false);
  glBindTexture(GL_TEXTURE_2D, backface_depth_tex);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screenW, screenH, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
  set_params(true);

  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backface_color_tex, 0
  );
  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, backface_depth_tex, 0
  );


  // update frontface fbo texture
  glBindFramebuffer(GL_FRAMEBUFFER, frntface_fbo);
  glBindTexture(GL_TEXTURE_2D, frntface_color_tex);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, screenW, screenH, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  set_params(false);
  glBindTexture(GL_TEXTURE_2D, frntface_depth_tex);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screenW, screenH, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
  set_params(true);

  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frntface_color_tex, 0
  );
  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, frntface_depth_tex, 0
  );
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::set_projection_matrix() {
  if (!init_headless) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(camera.v_fov(), camera.aspect_ratio(), camera.near_clip(),
                   camera.far_clip());
  }
}

string Application::name() { return "Scotty3D"; }

string Application::info() {
  if(noGUI) { return ""; }
  switch (mode) {
    case MODEL_MODE:
      return "MeshEdit";
      break;
    case ANIMATE_MODE:
      return "Animation";
      break;
    case RENDER_MODE:
    case VISUALIZE_MODE:
      return "PathTracer";
      break;
  }
}

void Application::load(SceneInfo *sceneInfo) {
  vector<Collada::Node> &nodes = sceneInfo->nodes;
  vector<DynamicScene::SceneLight *> lights;
  vector<DynamicScene::SceneObject *> objects;

  // save camera position to update camera control later
  CameraInfo *c;
  Vector3D c_pos = Vector3D();
  Vector3D c_dir = Vector3D();

  int len = nodes.size();
  for (int i = 0; i < len; i++) {
    Collada::Node &node = nodes[i];
    Collada::Instance *instance = node.instance;
    const Matrix4x4 &transform = node.transform;

    switch (instance->type) {
      case Collada::Instance::CAMERA:
        c = static_cast<CameraInfo *>(instance);
        c_pos = (transform * Vector4D(c_pos, 1)).to3D();
        c_dir = (transform * Vector4D(c->view_dir, 1)).to3D().unit();
        init_camera(*c, transform);
        break;
      case Collada::Instance::LIGHT: {
        lights.push_back(
            init_light(static_cast<LightInfo &>(*instance), transform));
        break;
      }
      case Collada::Instance::SPHERE:
        objects.push_back(
            init_sphere(static_cast<SphereInfo &>(*instance), transform));
        break;
      case Collada::Instance::POLYMESH:
        objects.push_back(
            init_polymesh(static_cast<PolymeshInfo &>(*instance), transform));
        break;
      case Collada::Instance::MATERIAL:
        init_material(static_cast<MaterialInfo &>(*instance));
        break;
    }
  }

  if (lights.size() == 0) {  // no lights, default use ambient_light
    LightInfo default_light = LightInfo();
    lights.push_back(new DynamicScene::AmbientLight(default_light));
  }
  scene = new DynamicScene::Scene(objects, lights);

  const BBox &bbox = scene->get_bbox();
  if (!bbox.empty()) {
    Vector3D target = bbox.centroid();
    canonical_view_distance = bbox.extent.norm() / 2 * 1.5;

    double view_distance = canonical_view_distance * 2;
    double min_view_distance = canonical_view_distance / 10.0;
    double max_view_distance = canonical_view_distance * 20.0;

    canonicalCamera.place(target, acos(c_dir.y), atan2(c_dir.x, c_dir.z),
                          view_distance, min_view_distance, max_view_distance);

    camera.place(target, acos(c_dir.y), atan2(c_dir.x, c_dir.z), view_distance,
                 min_view_distance, max_view_distance);

    set_scroll_rate();
  }

  // set default draw styles for meshEdit -
  scene->set_draw_styles(&defaultStyle, &hoverStyle, &selectStyle);

  // cerr << "==================================" << endl;
  // cerr << "CAMERA" << endl;
  // cerr << "      hFov: " << camera.hFov << endl;
  // cerr << "      vFov: " << camera.vFov << endl;
  // cerr << "        ar: " << camera.ar << endl;
  // cerr << "     nClip: " << camera.nClip << endl;
  // cerr << "     fClip: " << camera.fClip << endl;
  // cerr << "       pos: " << camera.pos << endl;
  // cerr << " targetPos: " << camera.targetPos << endl;
  // cerr << "       phi: " << camera.phi << endl;
  // cerr << "     theta: " << camera.theta << endl;
  // cerr << "         r: " << camera.r << endl;
  // cerr << "      minR: " << camera.minR << endl;
  // cerr << "      maxR: " << camera.maxR << endl;
  // cerr << "       c2w: " << camera.c2w << endl;
  // cerr << "   screenW: " << camera.screenW << endl;
  // cerr << "   screenH: " << camera.screenH << endl;
  // cerr << "screenDist: " << camera.screenDist<< endl;
  // cerr << "==================================" << endl;
}

void Application::init_camera(CameraInfo &cameraInfo,
                              const Matrix4x4 &transform) {
  camera.configure(cameraInfo, screenW, screenH);
  canonicalCamera.configure(cameraInfo, screenW, screenH);
  set_projection_matrix();
}

void Application::reset_camera() { camera.copy_placement(canonicalCamera); }

DynamicScene::SceneLight *Application::init_light(LightInfo &light,
                                                  const Matrix4x4 &transform) {
  switch (light.light_type) {
    case Collada::LightType::NONE:
      break;
    case Collada::LightType::AMBIENT:
      return new DynamicScene::AmbientLight(light);
    case Collada::LightType::DIRECTIONAL:
      return new DynamicScene::DirectionalLight(light, transform);
    case Collada::LightType::AREA:
      return new DynamicScene::AreaLight(light, transform);
    case Collada::LightType::POINT:
      return new DynamicScene::PointLight(light, transform);
    case Collada::LightType::SPOT:
      return new DynamicScene::SpotLight(light, transform);
    default:
      break;
  }
  return nullptr;
}

/**
 * The transform is assumed to be composed of translation, rotation, and
 * scaling, where the scaling is uniform across the three dimensions; these
 * assumptions are necessary to ensure the sphere is still spherical. Rotation
 * is ignored since it's a sphere, translation is determined by transforming the
 * origin, and scaling is determined by transforming an arbitrary unit vector.
 */
DynamicScene::SceneObject *Application::init_sphere(
    SphereInfo &sphere, const Matrix4x4 &transform) {
  const Vector3D &position = (transform * Vector4D(0, 0, 0, 1)).projectTo3D();
  double scale = (transform * Vector4D(1, 0, 0, 0)).to3D().norm();
  return new DynamicScene::Sphere(sphere, position, scale);
}

DynamicScene::SceneObject *Application::init_polymesh(
    PolymeshInfo &polymesh, const Matrix4x4 &transform) {
  return new DynamicScene::Mesh(polymesh, transform);
}

void Application::set_scroll_rate() {
  scroll_rate = canonical_view_distance / 10;
}

void Application::init_material(MaterialInfo &material) {
  // TODO : Support Materials.
}

void Application::cursor_event(float x, float y) {
  if (leftDown && !middleDown && !rightDown) {
    mouse1_dragged(x, y);
  } else if (!leftDown && !middleDown && rightDown) {
    mouse2_dragged(x, y);
  }

  mouseX = x;
  mouseY = y;
}

void Application::scroll_event(float offset_x, float offset_y) {
  // update_style();

  switch (mode) {
    case MODEL_MODE:
      switch (action) {
        case Action::Navigate:
        case Action::Edit:
        case Action::Bevel:
          camera.move_forward(-offset_y * scroll_rate);
          break;
        default:
          break;
      }
      break;
    case ANIMATE_MODE:
    case VISUALIZE_MODE:
      camera.move_forward(-offset_y * scroll_rate);
      break;
    case RENDER_MODE:
      break;
  }
}

void Application::mouse_event(int key, int event, unsigned char mods) {
  switch (event) {
    case EVENT_PRESS:
      switch (key) {
        case MOUSE_LEFT:
          mouse_pressed(LEFT);
          break;
        case MOUSE_RIGHT:
          mouse_pressed(RIGHT);
          break;
        case MOUSE_MIDDLE:
          mouse_pressed(MIDDLE);
          break;
      }
      break;
    case EVENT_RELEASE:
      switch (key) {
        case MOUSE_LEFT:
          mouse_released(LEFT);
          break;
        case MOUSE_RIGHT:
          mouse_released(RIGHT);
          break;
        case MOUSE_MIDDLE:
          mouse_released(MIDDLE);
          break;
      }
      break;
  }
}

void Application::char_event(unsigned int codepoint) {
  bool queued = false;

  switch_modes(codepoint);

  switch (mode) {
    case RENDER_MODE:
      switch (codepoint) {
        case 'w':
        case 'W':
          pathtracer->save_image(string("ScreenShot_") +
                                 string(to_string(time(NULL))) +
                                 string(".png"));
          break;
        case '+':
        case '=':
          pathtracer->stop();
          pathtracer->increase_area_light_sample_count();
          pathtracer->start_raytracing();
          break;
        case '-':
        case '_':
          pathtracer->stop();
          pathtracer->decrease_area_light_sample_count();
          pathtracer->start_raytracing();
          break;
        case '[':
        case ']':
          pathtracer->key_press(codepoint);
          break;
      }
      break;
    case VISUALIZE_MODE:
      switch (codepoint) {
        case ' ':
          reset_camera();
          break;
        default:
          pathtracer->key_press(codepoint);
      }
      break;
    case ANIMATE_MODE:
      switch (codepoint) {
        case 'c':
        case 'C':
          // 'c'reate joints
          clickedJoint = nullptr;
          toggle_create_joint_action();
          break;
        case 'e':
          if (action == Action::Object) scene->elementTransform->cycleMode();
          break;
        case 'g':
          toggleGhosted();
          break;
        case 'p':
        case 'P':
          to_pose_action();
          break;
        case 'S':
          symmetryEnabled = !symmetryEnabled;
          break;
        case 'X':
          symmetryAxis = Axis::X;
          break;
        case 'Y':
          symmetryAxis = Axis::Y;
          break;
        case 'Z':
          symmetryAxis = Axis::Z;
          break;
      }
      break;
    case MODEL_MODE:
      switch (codepoint) {
        case 'u':
        case 'U':
          scene->upsample_selected_mesh();
          break;
        case 'd':
        case 'D':
          scene->downsample_selected_mesh();
          break;
        case 'i':
        case 'I':
          // i for isotropic.
          scene->resample_selected_mesh();
          break;
        case 'f':
        case 'F':
          scene->flip_selected_edge();
          break;
        case 'p':
          scene->split_selected_edge();
          break;
        case 'c':
        case 'C':
          scene->collapse_selected_element();
          break;
        case 'n':
        case 'N':
          scene->selectNextHalfedge();
          break;
        case 't':
          scene->selectTwinHalfedge();
          break;
        case 'T':
          scene->triangulateSelection();
          break;
        case 's':
          // Catmull-Clark subdivision
          scene->subdivideSelection(true);
          break;
        case 'S':
          // linear subdivision
          scene->subdivideSelection(false);
          break;
        case 'h':
          scene->selectHalfedge();
          break;
        case ' ':
          to_navigate_action();
          break;
        case 'b':
          toggle_bevel_action();
          break;
        case 'e':
          cycle_edit_action();
          break;
        case 'w':
        case 'W':
          queueWrite();
          queued = true;
          break;
        case 'l':
        case 'L':
          queueLoad();
          queued = true;
          break;
        default:
          break;
      }
      break;
  }

  if (!queued) {
    executeFileOp(codepoint);
  }
  updateWidgets();
}

void Application::queueWrite() {
  readyWrite = true;
  cerr << "(Press a key 0-9 to write to a buffer)" << endl;
}

void Application::queueLoad() {
  readyLoad = true;
  cerr << "(Press a key 0-9 to load from a buffer)" << endl;
}

void Application::executeFileOp(int codepoint) {
  // If the user already pressed 'w' or 'l' (indicating that
  // they wanted to write or load a file) AND the next key
  // they pressed was a number, then we should go ahead and
  // write/load the file corresponding to the specified number.

  if (!readyLoad && !readyWrite) return;

  // Convert Unicode codepoint to integer.  (Probably a more
  // clever way to do this, but often a foolproof solution is
  // (much) better than a clever one...)
  int index = -1;
  switch (codepoint) {
    case '0':
      index = 0;
      break;
    case '1':
      index = 1;
      break;
    case '2':
      index = 2;
      break;
    case '3':
      index = 3;
      break;
    case '4':
      index = 4;
      break;
    case '5':
      index = 5;
      break;
    case '6':
      index = 6;
      break;
    case '7':
      index = 7;
      break;
    case '8':
      index = 8;
      break;
    case '9':
      index = 9;
      break;
    default:
      readyLoad = readyWrite = false;
      return;
      break;
  }

  stringstream filename;
  filename << "Scotty3D_buffer" << index << ".dae";

  if (readyWrite)
    writeScene(filename.str().c_str());
  else if (readyLoad)
    loadScene(filename.str().c_str());

  readyLoad = readyWrite = false;
}

void Application::setGhosted(bool isGhosted) {
  scene->selected.clear();
  scene->removeObject(scene->elementTransform);

  this->isGhosted = isGhosted;
  for (auto object : scene->objects) {
    object->isGhosted = isGhosted;
  }
}

void Application::toggleGhosted() {
  scene->selected.clear();
  scene->removeObject(scene->elementTransform);

  isGhosted = !isGhosted;
  for (auto object : scene->objects) {
    object->isGhosted = isGhosted;
  }
}

void Application::updateWidgets() {
  if (scene->selected.object == nullptr) {
    scene->removeObject(scene->elementTransform);
    return;
  }

  if (mode == MODEL_MODE) {
    if (action == Action::Edit) {
      if (scene->selected.object != scene->elementTransform &&
          scene->selected.element != nullptr) {
        scene->elementTransform->setTarget(scene->selected);
      }
    } else {
      scene->removeObject(scene->elementTransform);
      return;
    }
  } else if (mode == ANIMATE_MODE) {
    if (action == Action::Object) {
      if (scene->selected.object != scene->elementTransform) {
        scene->elementTransform->setTarget(scene->selected);
      }
    } else if (action == Action::Wave) {
      if (scene->selected.object != scene->elementTransform &&
          scene->selected.element != nullptr) {
        scene->elementTransform->setTarget(scene->selected);
      }
    }
  } else {
    scene->removeObject(scene->elementTransform);
    return;
  }
}

void Application::keyboard_event(int key, int event, unsigned char mods) {
  noGUI = false;
  switch (mode) {
    case ANIMATE_MODE:
      switch (key) {
        case GLFW_KEY_TAB:
          if (event == GLFW_PRESS && action == Action::Object)
            scene->elementTransform->cycleMode();
          break;
        case GLFW_KEY_L:
          if (event == GLFW_PRESS) timeline.action_loop();
          break;
        case GLFW_KEY_I:
          if (event == GLFW_PRESS) {
            action = Action::IK;
            ikTargets.clear();
          }
          break;
        case GLFW_KEY_UP:
          // One hour of video max
          if (event == GLFW_PRESS && timeline.getMaxFrame() < 216000) {
            // Give them another second
            timeline.makeLonger(60);
          }
          break;
        case GLFW_KEY_DOWN:
          if (event == GLFW_PRESS) {
            timeline.makeShorter(60);
          }
          break;
        case GLFW_KEY_SPACE:
          if (event == GLFW_PRESS && (action != Action::Raytrace_Video &&
                                      action != Action::Rasterize_Video)) {
            if (timeline.isCurrentlyPlaying())
              timeline.action_stop();
            else {
              timeline.action_play();
              scene->removeObject(scene->elementTransform);
            }
            setGhosted(!timeline.isCurrentlyPlaying() &&
                       action != Action::Wave);
          }
          break;
        case GLFW_KEY_LEFT:
          switch (mods) {
            case GLFW_MOD_SHIFT:
              if (event == GLFW_PRESS) timeline.action_goto_prev_key_frame();
              break;
            case GLFW_MOD_CONTROL:
              timeline.action_rewind();
              break;
            case GLFW_MOD_ALT:
              if (event == GLFW_PRESS) timeline.action_step_backward(1);
              break;
            default:
              timeline.action_step_backward(1);
              break;
          }
          break;
        case GLFW_KEY_RIGHT:
          switch (mods) {
            case GLFW_MOD_SHIFT:
              if (event == GLFW_PRESS) timeline.action_goto_next_key_frame();
              break;
            case GLFW_MOD_CONTROL:
              timeline.action_goto_end();
              break;
            case GLFW_MOD_ALT:
              if (event == GLFW_PRESS) timeline.action_step_forward(1);
              break;
            default:
              timeline.action_step_forward(1);
              break;
          }
          break;
        case GLFW_KEY_BACKSPACE:
        case GLFW_KEY_DELETE:
          if (event == GLFW_PRESS && !timeline.isCurrentlyPlaying()) {
            if (mods) {
              timeline.unmarkTime(timeline.getCurrentFrame());
              for (auto o : scene->objects) {
                DynamicScene::Mesh *m = dynamic_cast<DynamicScene::Mesh *>(o);
                if (m) m->unkeyframe(timeline.getCurrentFrame());
              }
            } else {
              if (action == Action::CreateJoint) {
                scene->erase_selected_joint();
                clickedJoint = nullptr;
              } else if (scene->selected.object != nullptr) {
                double t = timeline.getCurrentFrame();
                DynamicScene::Mesh *m =
                    dynamic_cast<DynamicScene::Mesh *>(scene->selected.object);
                if (m) m->unkeyframe(t);

                /* Determine if there are any other objects with knots here
                       * so we can unmark the timeline. */
                bool remaining = false;
                for (auto o : scene->objects) {
                  double t = timeline.getCurrentFrame();
                  auto v = o->positions(t);
                  if (o->positions.removeKnot(t)) {
                    remaining = true;
                    o->positions.setValue(t, v);
                    break;
                  }
                }
                if (remaining == false)
                  timeline.unmarkTime(timeline.getCurrentFrame());
              }
            }
          }
          break;
        case GLFW_KEY_O:
          to_object_action();
          break;
        case GLFW_KEY_U:
          if (event == GLFW_PRESS) useCapsuleRadius = !useCapsuleRadius;
          break;
        case GLFW_KEY_W:
          to_wave_action();
          break;
        case GLFW_KEY_Z:
          for (auto o : scene->objects) {
            if (o->getInfo()[0][0] == 'M') {  // Mesh
              ((DynamicScene::Mesh *)o)->resetWave();
            }
          }
          break;
        case GLFW_KEY_N:
          if (event == GLFW_PRESS) raytrace_video();
          break;
        case GLFW_KEY_T:
          if (event == GLFW_PRESS) rasterize_video();
          break;
        case GLFW_KEY_D:
          if (event == GLFW_PRESS) action = Action::BoneRadius;
          break;
        case GLFW_KEY_EQUAL:
          if (!(mods & GLFW_MOD_ALT) ||
              (mods & GLFW_MOD_ALT && event == GLFW_PRESS)) {
            if (mods & GLFW_MOD_SHIFT) {
              damping_factor += 0.001;
              if (damping_factor > 1) damping_factor = 1.0;
            } else {
              timestep += 0.01;
            }
          }
          break;
        case GLFW_KEY_MINUS:
          if (!(mods & GLFW_MOD_ALT) ||
              (mods & GLFW_MOD_ALT && event == GLFW_PRESS)) {
            if (mods & GLFW_MOD_SHIFT) {
              damping_factor -= 0.001;
              if (damping_factor < 0.0) damping_factor = 0.0;
            } else {
              timestep -= 0.01;
              if (timestep < EPS_D) timestep = 0.01;
            }
          }
          break;
        case GLFW_KEY_F:
          if (event == GLFW_PRESS) {
            integrator = Integrator::Forward_Euler;
          }
          break;
        case GLFW_KEY_S:
          if (event == GLFW_PRESS && !(mods & GLFW_MOD_SHIFT)) {
            integrator = Integrator::Symplectic_Euler;
          }
          break;
        case GLFW_KEY_K:
          if (event == GLFW_PRESS) {
            if (mods) {
              for (DynamicScene::SceneObject *o : scene->objects) {
                DynamicScene::Mesh *m = dynamic_cast<DynamicScene::Mesh *>(o);
                if (m) {
                  m->keyframe(timeline.getCurrentFrame());
                  timeline.markTime(timeline.getCurrentFrame());
                }
              }
            } else {
              DynamicScene::Mesh *m =
                  dynamic_cast<DynamicScene::Mesh *>(scene->selected.object);
              if (m) {
                m->keyframe(timeline.getCurrentFrame());
                timeline.markTime(timeline.getCurrentFrame());
              }
            }
          }
      }
      break;
    case RENDER_MODE:
      break;
    case VISUALIZE_MODE:
      break;
    case MODEL_MODE:
      switch (key) {
        case GLFW_KEY_BACKSPACE:
        case GLFW_KEY_DELETE:
          if (event == GLFW_PRESS) {
            scene->erase_selected_element();
          }
          break;
        case GLFW_KEY_TAB:
          if (event == GLFW_PRESS) {
            show_hud = !show_hud;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  if (lastEventWasModKey && event == GLFW_RELEASE) {
    scene->elementTransform->restoreLastMode();
  }

  lastEventWasModKey = false;
  if (mods && mode != ANIMATE_MODE) {
    lastEventWasModKey = true;

    switch (mods) {
      case GLFW_MOD_SHIFT:
        scene->elementTransform->setScale();
        break;
      case GLFW_MOD_CONTROL:
        scene->elementTransform->setTranslate();
        break;
      case GLFW_MOD_ALT:
        scene->elementTransform->setRotate();
        break;
      default:
        break;
    }
  }

  updateWidgets();
}

void Application::writeScene(const char *filename) {
  cerr << "Writing scene to file " << filename << endl;
  Collada::ColladaWriter::writeScene(*scene, filename);
  writeSkeleton(filename, scene);
}

void CMU462::Application::writeSkeleton(const char *filename,
                                        const DynamicScene::Scene *scene) {
  string filenameNoExt(filename);
  filenameNoExt = filenameNoExt.substr(0, filenameNoExt.find("."));
  int meshID = 0;
  for (auto object : scene->objects) {
    DynamicScene::Mesh *mesh = dynamic_cast<DynamicScene::Mesh *>(object);
    if (mesh) {
      stringstream outfilename;
      outfilename << filenameNoExt << "_" << meshID << "_skel.xml";
      mesh->skeleton->save(outfilename.str().c_str());
      meshID++;
    }
  }
}

void Application::loadScene(const char *filename) {
  cerr << "Loading scene from file " << filename << endl;

  Camera originalCamera = camera;
  Camera originalCanonicalCamera = canonicalCamera;

  Collada::SceneInfo *sceneInfo = new Collada::SceneInfo();
  if (Collada::ColladaParser::load(filename, sceneInfo) < 0) {
    cerr << "Warning: scene file failed to load." << endl;
    delete sceneInfo;
    return;
  }
  load(sceneInfo);
  loadSkeleton(filename, scene);

  camera = originalCamera;
  canonicalCamera = originalCanonicalCamera;
}

void CMU462::Application::loadSkeleton(const char *filename,
                                       DynamicScene::Scene *scene) {
  string filenameNoExt(filename);
  filenameNoExt = filenameNoExt.substr(0, filenameNoExt.find("."));
  int meshID = 0;
  for (auto object : scene->objects) {
    DynamicScene::Mesh *mesh = dynamic_cast<DynamicScene::Mesh *>(object);
    if (mesh) {
      stringstream loadfilename;
      loadfilename << filenameNoExt << "_" << meshID << "_skel.xml";
      set<double> knotTimes = mesh->skeleton->load(loadfilename.str().c_str());
      // Add all joints to the scene
      for (auto joint : mesh->skeleton->joints) {
        scene->addObject(joint);
      }
      // Mark all knots on the timeline
      for (auto knotTime : knotTimes) {
        timeline.markTime(knotTime);
      }
      meshID++;
    }
  }
}

void Application::to_navigate_action() { action = Action::Navigate; }

void Application::toggle_bevel_action() {
  if (action != Action::Bevel) {
    action = Action::Bevel;
  } else {
    action = Action::Navigate;
  }
}

void Application::toggle_create_joint_action() {
  if (action != Action::CreateJoint) {
    action = Action::CreateJoint;
    setGhosted(true);
    scene->removeObject(scene->elementTransform);
    scene->selected.object = nullptr;
  } else {
    action = Action::Object;
    setGhosted(false);
  }
}

void Application::cycle_edit_action() {
  if (action != Action::Edit) {
    action = Action::Edit;
    setupElementTransformWidget();
  } else {
    scene->elementTransform->cycleMode();
  }
  updateWidgets();
}

Vector3D Application::getMouseProjection(double dist) {
  // get projection matrix from OpenGL stack.
  GLdouble projection[16];
  glGetDoublev(GL_PROJECTION_MATRIX, projection);
  Matrix4x4 projection_matrix(projection);

  // get view matrix from OpenGL stack.
  GLdouble modelview[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
  Matrix4x4 modelview_matrix(modelview);

  // ray in clip coordinates
  double x = mouseX * 2 / screenW - 1;
  double y = screenH - mouseY;  // y is upside down
  y = y * 2 / screenH - 1;

  Vector4D ray_clip(x, y, -1.0, 1.0);
  // ray in eye coordinates
  Vector4D ray_eye = projection_matrix.inv() * ray_clip;

  // ray is into the screen and not a point.
  ray_eye.z = -1.0;
  ray_eye.w = 0.0;

  GLfloat z1, z2;
  const float zNear = camera.near_clip();
  const float zFar = camera.far_clip();
  glBindFramebuffer(GL_READ_FRAMEBUFFER, frntface_fbo);
  glReadPixels(mouseX, screenH-mouseY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z1);
  z1 = z1 * 2.f - 1.f;
  z1 = 2.f * zNear * zFar / (zFar + zNear - z1 * (zFar - zNear));
  glBindFramebuffer(GL_READ_FRAMEBUFFER, backface_fbo);
  glReadPixels(mouseX, screenH-mouseY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z2);
  z2 = z2 * 2.f - 1.f;
  z2 = 2.f * zNear * zFar / (zFar + zNear - z2 * (zFar - zNear));
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

  bool invalid = (z1 <= zNear || z2 <= zNear || z1 >= zFar || z2 >= zFar);

  // ray in world coordinates
  Vector4D ray_wor4 = modelview_matrix * ray_eye;
  Vector3D ray_wor(ray_wor4.x, ray_wor4.y, ray_wor4.z);

  Vector3D ray_orig(camera.position());

  double t = dot(ray_orig, -ray_wor);
  if(std::isfinite(dist)) {
    // If a distance was given, use that instead
    ray_wor = ray_wor.unit();
    t = dist;
  } else if(!invalid) {
    // The current ray  is not a unit vector - it is normalized in z
    // so we can simply move along that (scaled) ray by the viewspace
    // z value
    t = z1 + (z2 - z1) / 2.f; // average of in/out depth
  }

  Vector3D intersect = ray_orig + t * ray_wor;

  return intersect;
}

void Application::to_wave_action() {
  scene->elementTransform->exitObjectMode();
  scene->elementTransform->enterTransformedMode();
  scene->elementTransform->setTranslate();
  scene->removeObject(scene->elementTransform);
  action = Action::Wave;
  setGhosted(false);
}

void Application::to_object_action() {
  scene->elementTransform->enterObjectMode();
  scene->removeObject(scene->elementTransform);
  action = Action::Object;
  setGhosted(true);
}

void Application::to_pose_action() {
  action = Action::Pose;
  setGhosted(true);
}

void Application::mouse_pressed(e_mouse_button b) {

  Vector2D p(mouseX, screenH - mouseY);
  scene->getHoveredObject(p);

  switch (b) {
    case LEFT:
      leftDown = true;
      if (mode == MODEL_MODE) {
        selectHovered();
        if (action == Action::Bevel) {
          scene->bevel_selected_element();
        }
      } else if (mode == ANIMATE_MODE) {
        draggingTimeline = false;
        if (timeline.mouse_over(mouseX, mouseY)) {
          timeline.mouse_click(mouseX, mouseY);
          draggingTimeline = true;
        } else if (action == Action::CreateJoint) {
          // See if hovered object is a valid mesh or joint.
          DynamicScene::Mesh *mesh =
              dynamic_cast<DynamicScene::Mesh *>(scene->hovered.object);
          DynamicScene::Joint *joint =
              dynamic_cast<DynamicScene::Joint *>(scene->hovered.object);
          if (joint != nullptr) {
            selectHovered();
            clickedJoint = joint;
          } else if (mesh != nullptr) {
            selectHovered();
            clickedJoint = mesh->skeleton->root;
          }
          // A joint is already selected. Create a new joint connected to it.
          else if (clickedJoint != nullptr) {
            Vector3D clickPos = getMouseProjection();
            auto skeleton = clickedJoint->skeleton;
            auto newJoint = skeleton->createNewJoint(clickedJoint, clickPos);
            scene->addObject(newJoint);

            // If symmetry is enabled, create a mirrored joint
            if(symmetryEnabled) {
              auto p = clickedJoint;
              if(p->mirror) p = p->mirror;
              auto mPos = clickPos;
              switch(symmetryAxis) {
                case Axis::X: mPos.x *= -1; break;
                case Axis::Y: mPos.y *= -1; break;
                case Axis::Z: mPos.z *= -1; break;
              }
              auto joint2 = skeleton->createNewJoint(p, mPos);
              scene->addObject(joint2);

              newJoint->mirror = joint2;
              joint2->mirror = newJoint;
            }

            clickedJoint = newJoint;
          }
        } else if (!scene->elementTransform->getIsTransforming()) {
          if (timeline.isCurrentlyPlaying()) return;
          selectHovered();
          if (action == Action::Wave) {
            if (scene->selected.element != nullptr &&
                scene->selected.element->getInfo()[0][0] != 'V') {
              scene->selected.element = nullptr;
              scene->selected.object = nullptr;
            }
          }
        }
      }
      break;
    case RIGHT:
      rightDown = true;
      break;
    case MIDDLE:
      middleDown = true;
      break;
  }
  updateWidgets();
}

void Application::selectHovered() {
  scene->selected = scene->hovered;

  scene->elementTransform->setClickPosition(Vector2D(mouseX, mouseY));

  setupElementTransformWidget();

  scene->edited.clear();
}

void Application::setupElementTransformWidget() {
  switch (mode) {
    case RENDER_MODE:
    case VISUALIZE_MODE:
      break;
    case MODEL_MODE:
      scene->elementTransform->exitObjectMode();
      scene->elementTransform->exitTransformedMode();
      scene->elementTransform->exitPoseMode();
      if (scene->selected.element != nullptr  && scene->selected.object != scene->elementTransform) {
        scene->elementTransform->setTarget(scene->selected);
        scene->addObject(scene->elementTransform);
      }
      break;
    case ANIMATE_MODE:
      if (action == Action::Wave) {
        scene->elementTransform->exitObjectMode();
        scene->elementTransform->exitPoseMode();
        scene->elementTransform->enterTransformedMode();
        if (scene->selected.element != nullptr && scene->selected.object != scene->elementTransform) {
          scene->elementTransform->setTarget(scene->selected);
          scene->addObject(scene->elementTransform);
        }
      } else if (action == Action::Object) {
        scene->elementTransform->enterObjectMode();
        scene->elementTransform->exitPoseMode();
        scene->elementTransform->exitTransformedMode();
        if (scene->selected.object != nullptr && scene->selected.object != scene->elementTransform) {
          scene->elementTransform->setTarget(scene->selected);
          scene->addObject(scene->elementTransform);
        }
      } else if (action == Action::Pose) {
        scene->elementTransform->enterObjectMode();
        scene->elementTransform->exitTransformedMode();
        if (scene->selected.object != nullptr && scene->selected.object != scene->elementTransform) {
          scene->elementTransform->setTarget(scene->selected);
          scene->addObject(scene->elementTransform);

          DynamicScene::Joint *joint =
              dynamic_cast<DynamicScene::Joint *>(scene->selected.object);

          if (joint != nullptr) {
            bool isRoot = joint == joint->skeleton->root;
            scene->elementTransform->enterPoseMode();
            if (isRoot)
              // Only allow root translation
              scene->elementTransform->setTranslate();
            else
              // Other joints are only allowed to be rotated
              scene->elementTransform->setRotate();
          } else {
            scene->elementTransform->exitPoseMode();
          }

          scene->elementTransform->updateGeometry(); // enterPoseMode affects updateGeometry
        }
      }
      break;
  }
}

void Application::mouse_released(e_mouse_button b) {
  switch (b) {
    case LEFT:
      leftDown = false;
      draggingTimeline = false;
      scene->elementTransform->updateGeometry();
      scene->elementTransform->onMouseReleased();
      break;
    case RIGHT:
      rightDown = false;
      break;
    case MIDDLE:
      middleDown = false;
      break;
  }

  updateWidgets();
}

/*
  When in edit mode and there is a selection, move the selection.
  When in visualization mode, rotate.
*/
void Application::mouse1_dragged(float x, float y) {
  if (mode == RENDER_MODE) {
    return;
  }
  float dx = (x - mouseX);
  float dy = (y - mouseY);

  if (mode == MODEL_MODE) {
    switch (action) {
      case (Action::Navigate):
        camera.rotate_by(dy * (PI / screenH), dx * (PI / screenW));
        break;
      case (Action::Edit):
        if (scene->has_selection()) {
          dragSelection(x, y, dx, dy, get_world_to_3DH());
        } else  // if nothing is selected, we may as well allow the user to
                // rotate the view...
        {
          camera.rotate_by(dy * (PI / screenH), dx * (PI / screenW));
        }
        break;
      case (Action::Bevel):
        if (scene->has_selection()) {
          scene->update_bevel_amount(dx, dy);
        } else  // if nothing is selected, we may as well allow the user to
                // rotate the view...
        {
          camera.rotate_by(dy * (PI / screenH), dx * (PI / screenW));
        }
        break;
      default:
        break;
    }
  } else if (mode == ANIMATE_MODE) {
    // Don't drag selected joint or mesh if in CreateJoint mode
    if (action == Action::CreateJoint) {
    } else if (action == Action::BoneRadius) {
      DynamicScene::Joint *joint =
          dynamic_cast<DynamicScene::Joint *>(scene->hovered.object);
      if (joint != nullptr) {
        joint->capsuleRadius += dy * 0.01;
      }
    } else if (draggingTimeline || timeline.isCurrentlyPlaying()) {
      timeline.mouse_click(x, y);
    } else if (action == Action::Object) {
      if (scene->has_selection()) {
        double t = timeline.getCurrentFrame();
        dragSelection(x, y, dx, dy, get_world_to_3DH());
        auto target = (scene->selected.object != scene->elementTransform)
                          ? (scene->selected.object)
                          : (scene->elementTransform->target.object);

        if (scene->elementTransform->mode ==
            DynamicScene::XFormWidget::Mode::Translate) {
          target->positions.setValue(t, target->position);
        } else if (scene->elementTransform->mode ==
                   DynamicScene::XFormWidget::Mode::Scale) {
          target->scales.setValue(t, target->scale);
        } else if (scene->elementTransform->mode ==
                   DynamicScene::XFormWidget::Mode::Rotate) {
          target->rotations.setValue(t, target->rotation);
        }
        timeline.markTime(t);
      } else {
        camera.rotate_by(dy * (PI / screenH), dx * (PI / screenW));
      }
    } else if (action == Action::Pose) {
      if (scene->has_selection()) {
        double t = timeline.getCurrentFrame();
        auto joint =
            dynamic_cast<DynamicScene::Joint *>(scene->selected.object);
        if (joint == nullptr) dragSelection(x, y, dx, dy, get_world_to_3DH());
        auto target = (scene->selected.object != scene->elementTransform)
                          ? (scene->selected.object)
                          : (scene->elementTransform->target.object);
        DynamicScene::Joint *m = dynamic_cast<DynamicScene::Joint *>(target);
        if (m) m->skeleton->mesh->keyframe(t);
        timeline.markTime(t);
      } else {
        camera.rotate_by(dy * (PI / screenH), dx * (PI / screenW));
      }
    } else if (action == Action::Wave) {
      if (scene->has_selection()) {
        Vertex *v = (Vertex *)scene->elementTransform->target.element;
        Vector3D initialPos = v->position;
        dragSelection(x, y, dx, dy, get_world_to_3DH());
        float dPos = dot(v->position - initialPos, v->normal());
        v->position = initialPos;
        map<HalfedgeIter, double> seen;
        v->smoothNeighborhood(dPos, seen, 5);
        // Preserve volume
        DynamicScene::Mesh *mesh = dynamic_cast<DynamicScene::Mesh *>(
            scene->elementTransform->target.object);
        if (mesh != nullptr) {
          HalfedgeMesh &halfEdgeMesh = mesh->mesh;
          double avg = 0.0;
          int i = 0;
          for (VertexIter v = halfEdgeMesh.verticesBegin();
               v != halfEdgeMesh.verticesEnd(); v++) {
            avg += v->offset;
            i++;
          }
          avg /= i;

          for (VertexIter v = halfEdgeMesh.verticesBegin();
               v != halfEdgeMesh.verticesEnd(); v++) {
            v->offset = v->offset - avg;
          }
        }
      } else {
        camera.rotate_by(dy * (PI / screenH), dx * (PI / screenW));
      }
    }
  } else {
    camera.rotate_by(dy * (PI / screenH), dx * (PI / screenW));
  }

  updateWidgets();
}

/*
  When the mouse is dragged with the right button held down, translate.
*/
void Application::mouse2_dragged(float x, float y) {
  if (mode == RENDER_MODE) return;
  if (mode == ANIMATE_MODE && action == Action::IK) return;
  float dx = (x - mouseX);
  float dy = (y - mouseY);
  // don't negate y because up is down.
  camera.move_by(-dx, dy, canonical_view_distance);

  updateWidgets();
}

void Application::switch_modes(unsigned int key) {
  switch (key) {
    case 'a':
    case 'A':
      to_animate_mode();
      setupElementTransformWidget();
      break;
    case 'm':
    case 'M':
      to_model_mode();
      setupElementTransformWidget();
      break;
    case 'r':
    case 'R':
      to_render_mode();
      setupElementTransformWidget();
      break;
    case 'v':
    case 'V':
      to_visualize_mode();
      setupElementTransformWidget();
      break;
    default:
      break;
  }
}

static void reset_vertex_positions(DynamicScene::Mesh *m, bool to_animate_mode) {
  auto begin = m->mesh.verticesBegin();
  auto end = m->mesh.verticesEnd();
  for (auto cur = begin; cur != end; ++cur) {
    if (to_animate_mode)
      // Reset the vertex positions to their bind positions so we can edit the model
      cur->bindPosition = cur->position;
    else
      // Set the bind positions to their (potentially modified) positions in edit mode so they can get skinned
      cur->position = cur->bindPosition;
  }
}

void Application::to_model_mode() {
  if (mode == MODEL_MODE) return;
  pathtracer->stop();
  pathtracer->clear();
  scene->selected.clear();

  for (auto o : scene->objects) {
    DynamicScene::Mesh *m = dynamic_cast<DynamicScene::Mesh *>(o);
    if (m != nullptr) {  // Mesh
      m->resetWave();

      reset_vertex_positions(m, false);
    }
  }

  mode = MODEL_MODE;

  action = Action::Navigate;
  setGhosted(false);
}

void Application::to_render_mode() {
  if (mode == RENDER_MODE) return;
  scene->selected.clear();
  scene->triangulateSelection();
  set_up_pathtracer();
  pathtracer->stop();
  pathtracer->start_raytracing();
  mode = RENDER_MODE;
  setGhosted(false);
}

void Application::to_animate_mode() {
  if (mode == ANIMATE_MODE) return;
  scene->selected.clear();
  pathtracer->stop();
  pathtracer->clear();
  mode = ANIMATE_MODE;
  to_object_action();
  for (DynamicScene::SceneObject *o : scene->objects) {
    DynamicScene::Mesh *m = dynamic_cast<DynamicScene::Mesh *>(o);
    if (m != nullptr) {
      reset_vertex_positions(m, true);

      scene->addObject(m->skeleton);
    }
  }
  integrator = Integrator::Forward_Euler;
  setGhosted(false);
}

void Application::to_visualize_mode() {
  if (mode == VISUALIZE_MODE) return;
  scene->selected.clear();
  set_up_pathtracer();
  pathtracer->stop();
  pathtracer->start_visualizing();
  mode = VISUALIZE_MODE;
  setGhosted(false);
}

void Application::set_up_pathtracer() {
  if (mode != MODEL_MODE && mode != ANIMATE_MODE) return;
  pathtracer->set_camera(&camera);
  if (action == Action::Raytrace_Video) {
    pathtracer->set_scene(
        scene->get_transformed_static_scene(timeline.getCurrentFrame()));
  } else {
    pathtracer->set_scene(scene->get_static_scene());
  }
  pathtracer->set_frame_size(screenW, screenH);
}

void Application::rasterize_video() {
  noGUI = true;
  scene->removeObject(scene->elementTransform);
  static string videoPrefix;

  setGhosted(false);

  if (action == Action::Rasterize_Video) {
    char num[32];
    sprintf(num, "%04d", timeline.getCurrentFrame());
    string fname = videoPrefix + string(num) + string(".png");

    unsigned char *colors = new unsigned char[screenW * screenH * 4];

    float *red =  new float[screenW * screenH];
    float *green =  new float[screenW * screenH];
    float *blue =  new float[screenW * screenH];

    glReadBuffer( GL_FRONT );
    glReadPixels(0, 0, screenW, screenH, GL_RED, GL_FLOAT, red);
    glReadPixels(0, 0, screenW, screenH, GL_GREEN, GL_FLOAT, green);
    glReadPixels(0, 0, screenW, screenH, GL_BLUE, GL_FLOAT, blue);
    
    for (int i = 0; i < screenW * screenH; i++) {
      colors[4*i    ] = (unsigned char)(red[i] * 255);
      colors[4*i + 1] = (unsigned char)(green[i] * 255);
      colors[4*i + 2] = (unsigned char)(blue[i] * 255);
      colors[4*i + 3] = 255;
    } 

    uint32_t *frame = (uint32_t *)colors;
    size_t w = screenW;
    size_t h = screenH;
    uint32_t *frame_out = new uint32_t[w * h];
    for (size_t i = 0; i < h; ++i) {
      memcpy(frame_out + i * w, frame + (h - i - 1) * w, 4 * w);
    }

    fprintf(stderr, "[Animator] Saving to file: %s... ", fname.c_str());
    lodepng::encode(fname, (unsigned char *)frame_out, w, h);
    fprintf(stderr, "Done!\n");

    if (timeline.getCurrentFrame() == timeline.getMaxFrame()) {
      timeline.action_rewind();
      cout << "Done rendering video!" << endl;
      action = Action::Object;
    }
  } else {
    action = Action::Rasterize_Video;
    videoPrefix = string("Video_") + to_string(time(NULL)) + string("_");
    timeline.action_rewind();
    timeline.action_play();
  }
}

void Application::raytrace_video() {
  static string videoPrefix;

  if (action == Action::Raytrace_Video) {
    if (pathtracer->is_done()) {
      char num[32];
      sprintf(num, "%04d", timeline.getCurrentFrame());
      pathtracer->save_image(videoPrefix + num + string(".png"));
      timeline.step();

      if (timeline.getCurrentFrame() == timeline.getMaxFrame()) {
        timeline.action_stop();
        timeline.action_rewind();
        cout << "Done rendering video!" << endl;
        action = Action::Object;
        return;
      }
      pathtracer->stop();
      pathtracer->clear();
      set_up_pathtracer();
      pathtracer->start_raytracing();
    }
  } else {
    action = Action::Raytrace_Video;
    scene->triangulateSelection();
    videoPrefix = string("Video_") + to_string(time(NULL)) + string("_");
    timeline.action_rewind();
    timeline.action_play();
    char num[32];
    pathtracer->stop();
    pathtracer->clear();
    set_up_pathtracer();
    pathtracer->start_raytracing();
  }
}

Matrix4x4 Application::get_world_to_3DH() {
  Matrix4x4 P, V, M;
  glGetDoublev(GL_PROJECTION_MATRIX, &P(0, 0));
  auto sel = scene->selected.object;
  if(sel) {
    if(sel == scene->elementTransform)
      M = scene->elementTransform->target.object->getTransformation();
    else
      M = sel->getTransformation();
  }
  else
    M = Matrix4x4::identity();
  V = camera.getTransformation();
  return P * V * M;
}

void Application::dragSelection(float x, float y, float dx, float dy,
                                const Matrix4x4 &modelViewProj) {
  if (scene->selected.object == nullptr) return;

  dx *= 2. / screenW;
  dy *= -2. / screenH;

  if (scene->selected.element) {
    scene->selected.element->translate(dx, dy, modelViewProj);
  } else {
    scene->selected.object->drag(x, y, dx, dy, modelViewProj);
  }
}

inline void Application::draw_string(float x, float y, string str, size_t size,
                                     const Color &c) {
  int line_index = textManager.add_line((x * 2 / screenW) - 1.0,
                                        (-y * 2 / screenH) + 1.0, str, size, c);
  messages.push_back(line_index);
}

void Application::draw_coordinates() {
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glLineWidth(2.);

  glBegin(GL_LINES);
  glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
  glVertex3i(0, 0, 0);
  glVertex3i(1, 0, 0);

  glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
  glVertex3i(0, 0, 0);
  glVertex3i(0, 1, 0);

  glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
  glVertex3i(0, 0, 0);
  glVertex3i(0, 0, 1);

  glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
  for (int x = 0; x <= 8; ++x) {
    glVertex3i(x - 4, 0, -4);
    glVertex3i(x - 4, 0, 4);
  }
  for (int z = 0; z <= 8; ++z) {
    glVertex3i(-4, 0, z - 4);
    glVertex3i(4, 0, z - 4);
  }
  glEnd();

  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);
}

void Application::draw_hud() {
  textManager.clear();
  messages.clear();

  const size_t size = 16;
  const float x0 = use_hdpi ? screenW - 300 * 2 : screenW - 300;
  const float y0 = use_hdpi ? 128 : 64;
  const int inc = use_hdpi ? 48 : 24;
  float y = y0 + inc - size;

  // No selection --> no messages.
  if (!scene->has_selection()) {
    draw_string(x0, y, "No mesh feature is selected", size, text_color);
    y += inc;
  } else {
    Info selectionInfo = scene->getSelectionInfo();
    for (const string &s : selectionInfo) {
      size_t split = s.find_first_of(":");
      if (split != string::npos) {
        split++;
        string s1 = s.substr(0, split);
        string s2 = s.substr(split);
        draw_string(x0, y, s1, size, text_color);
        draw_string(x0 + (use_hdpi ? 150 : 75), y, s2, size, text_color);
      } else {
        draw_string(x0, y, s, size, text_color);
      }
      y += inc;
    }
  }

  // -- First draw a lovely black rectangle.

  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(0, 0, screenW, screenH);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, screenW, screenH, 0, 0, 1);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(0, 0, -1);

  // -- Black with opacity .8;

  glColor4f(0.0, 0.0, 0.0, 0.8);

  float min_x = x0 - 32;
  float min_y = y0 - 32;
  float max_x = screenW;
  float max_y = y;

  float z = 0.0;

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  glBegin(GL_QUADS);

  glVertex3f(min_x, min_y, z);
  glVertex3f(min_x, max_y, z);
  glVertex3f(max_x, max_y, z);
  glVertex3f(max_x, min_y, z);
  glEnd();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopAttrib();

  glEnable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);

  textManager.render();
}

void Application::draw_action() {
  if (mode != MODEL_MODE && mode != ANIMATE_MODE) return;
  if (mode == ANIMATE_MODE && timeline.isCurrentlyPlaying()) return;

  textManager.clear();
  messages.clear();

  const size_t size = 16;
  const float x0 = use_hdpi ? 48 : 24;
  const float y0 = use_hdpi ? 80 : 40;
  const int inc = use_hdpi ? 48 : 24;
  float y = y0 + inc - size;

  stringstream actionString;
  actionString << "ACTION: ";
  switch (action) {
    case (Action::Navigate):
      actionString << "Navigate";
      break;
    case (Action::Edit):
      actionString << "Edit";
      break;
    case (Action::Bevel):
      actionString << "Bevel";
      break;
    case (Action::CreateJoint):
      actionString << "Create Joint";
      break;
    case (Action::Wave):
      actionString << "Wave";
      break;
    case (Action::Object):
      actionString << "Object Transformation";
      break;
    case (Action::Pose):
      actionString << "Pose";
      break;
    case (Action::IK):
      actionString << "Inverse Kinematics";
      break;
    case (Action::BoneRadius):
      actionString << "Change Threshold Radius";
      break;
    default:
      actionString << "(none)";
      break;
  };

  Color action_color(0.3, .7, 0.3);
  draw_string(x0, y, actionString.str(), size, action_color);

  if (mode == ANIMATE_MODE) {
    stringstream integrator_string;
    integrator_string << "Integrator: ";
    switch (integrator) {
      case Integrator::Forward_Euler:
        integrator_string << "Forward Euler";
        break;
      case Integrator::Symplectic_Euler:
        integrator_string << "Symplectic Euler";
        break;
    }
    Color integrator_color(0.7, 0.3, 0.7);
    draw_string(x0, y + 40, integrator_string.str(), size, integrator_color);
    stringstream timestep_string;
    timestep_string << "Timestep: " << timestep;
    draw_string(x0, y + 80, timestep_string.str(), size, integrator_color);
    stringstream damping_string;
    damping_string << "Damping Factor: " << damping_factor;
    draw_string(x0, y + 120, damping_string.str(), size, integrator_color);
    stringstream lbs_string;
    lbs_string << "Linear Blend Skinning: "
               << (useCapsuleRadius ? "Threshold" : "Naive");
    draw_string(x0, y + 160, lbs_string.str(), size, integrator_color);

    if(action == Action::CreateJoint) {
      stringstream sym_string;
      sym_string << "Joint Symmetry: " << (symmetryEnabled ? "Enabled" : "Disabled");
      draw_string(x0, y + 200, sym_string.str(), size, integrator_color);
      stringstream axis_string;
      axis_string << "Symmetry Axis: " << ((char)symmetryAxis);
      draw_string(x0, y + 240, axis_string.str(), size, integrator_color);
    }
  }

  // // No selection --> no messages.
  // if (!scene->has_selection()) {
  //   draw_string(x0, y, "No mesh feature is selected", size, text_color);
  //   y += inc;
  // } else {
  //   DynamicScene::SelectionInfo selectionInfo = scene->get_selection_info();
  //   for (const string& s : selectionInfo) {
  //     size_t split = s.find_first_of(":");
  //     if (split != string::npos) {
  //       split++;
  //       string s1 = s.substr(0,split);
  //       string s2 = s.substr(split);
  //       draw_string(x0, y, s1, size, text_color);
  //       draw_string(x0 + (use_hdpi ? 150 : 75 ), y, s2, size, text_color);
  //     } else {
  //       draw_string(x0, y, s, size, text_color);
  //     }
  //     y += inc;
  //   }
  // }

  // // -- First draw a lovely black rectangle.

  // glPushAttrib(GL_VIEWPORT_BIT);
  // glViewport(0, 0, screenW, screenH);

  // glMatrixMode(GL_PROJECTION);
  // glPushMatrix();
  // glLoadIdentity();
  // glOrtho(0, screenW, screenH, 0, 0, 1);

  // glMatrixMode(GL_MODELVIEW);
  // glPushMatrix();
  // glLoadIdentity();
  // glTranslatef(0, 0, -1);

  // // -- Black with opacity .8;

  // glColor4f(0.0, 0.0, 0.0, 0.8);

  // float min_x = x0 - 32;
  // float min_y = y0 - 32;
  // float max_x = screenW;
  // float max_y = y;

  // float z = 0.0;

  // glDisable(GL_DEPTH_TEST);
  // glDisable(GL_LIGHTING);

  // glBegin(GL_QUADS);

  // glVertex3f(min_x, min_y, z);
  // glVertex3f(min_x, max_y, z);
  // glVertex3f(max_x, max_y, z);
  // glVertex3f(max_x, min_y, z);
  // glEnd();

  // glMatrixMode(GL_PROJECTION);
  // glPopMatrix();

  // glMatrixMode(GL_MODELVIEW);
  // glPopMatrix();

  // glPopAttrib();

  // glEnable(GL_LIGHTING);
  // glEnable(GL_DEPTH_TEST);

  textManager.render();
}

void Application::render_scene(std::string saveFileLocation) {

  set_up_pathtracer();
  pathtracer->start_raytracing();

  auto is_done = [this]() {
      if(init_headless)
          return pathtracer->is_done_headless();
      else
          return pathtracer->is_done();
  };

  while(!is_done()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  pathtracer->save_image(saveFileLocation);
}

}  // namespace CMU462
