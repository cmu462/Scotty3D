#ifndef CMU462_RAYTRACER_H
#define CMU462_RAYTRACER_H

#include <stack>
#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>

#include "CMU462/timer.h"

#include "bvh.h"
#include "camera.h"
#include "sampler.h"
#include "image.h"
#include "work_queue.h"

#include "static_scene/scene.h"
using CMU462::StaticScene::Scene;

#include "static_scene/environment_light.h"
using CMU462::StaticScene::EnvironmentLight;

using CMU462::StaticScene::BVHNode;
using CMU462::StaticScene::BVHAccel;

namespace CMU462 {

struct WorkItem {
  // Default constructor.
  WorkItem() : WorkItem(0, 0, 0, 0) {}

  WorkItem(int x, int y, int w, int h)
      : tile_x(x), tile_y(y), tile_w(w), tile_h(h) {}

  int tile_x;
  int tile_y;
  int tile_w;
  int tile_h;
};

/**
 * A pathtracer with BVH accelerator and BVH visualization capabilities.
 * It is always in exactly one of the following states:
 * -> INIT: is missing some data needed to be usable, like a camera or scene.
 * -> READY: fully configured, but not rendering.
 * -> VISUALIZE: visualizatiNG BVH aggregate.
 * -> RENDERING: rendering a scene.
 * -> DONE: completed rendering a scene.
 */
class PathTracer {
 public:
  /**
   * Default constructor.
   * Creates a new pathtracer instance.
   */
  PathTracer(size_t ns_aa = 1, size_t max_ray_depth = 4,
             size_t ns_area_light = 1, size_t ns_diff = 1, size_t ns_glsy = 1,
             size_t ns_refr = 1, size_t num_threads = 1,
             HDRImageBuffer* envmap = NULL);

  /**
   * Destructor.
   * Frees all the internal resources used by the pathtracer.
   */
  ~PathTracer();

  /**
   * If in the INIT state, configures the pathtracer to use the given scene. If
   * configuration is done, transitions to the READY state.
   * This DOES take ownership of the scene, and therefore deletes it if a new
   * scene is later passed in.
   * \param scene pointer to the new scene to be rendered
   */
  void set_scene(Scene* scene);

  /**
   * If in the INIT state, configures the pathtracer to use the given camera. If
   * configuration is done, transitions to the READY state.
   * This DOES NOT take ownership of the camera, and doesn't delete it ever.
   * \param camera the camera to use in rendering
   */
  void set_camera(Camera* camera);

  /**
   * Sets the pathtracer's frame size. If in a running state (VISUALIZE,
   * RENDERING, or DONE), transitions to READY b/c a changing window size
   * would invalidate the output. If in INIT and configuration is done,
   * transitions to READY.
   * \param width width of the frame
   * \param height height of the frame
   */
  void set_frame_size(size_t width, size_t height);

  /**
   * Update result on screen.
   * If the pathtracer is in RENDERING or DONE, it will display the result in
   * its frame buffer. If the pathtracer is in VISUALIZE mode, it will draw
   * the BVH visualization with OpenGL.
   */
  void update_screen();

  /**
   * Transitions from any running state to READY.
   */
  void stop();

  /**
   * If the pathtracer is in READY, delete all internal data, transition to
   * INIT.
   */
  void clear();

  /**
   * If the pathtracer is in READY, transition to VISUALIZE.
   */
  void start_visualizing();

  /**
   * If the pathtracer is in READY, transition to RENDERING.
   */
  void start_raytracing();

  /**
   * If the pathtracer is in VISUALIZE, handle key presses to traverse the bvh.
   */
  void key_press(int key);

  /**
   * Increase the pathtracer's area light sample count parameter by 2X
   */
  void increase_area_light_sample_count();

  /**
   * Decrease the pathtracer's area light sample count parameter by 2X
   */
  void decrease_area_light_sample_count();

  /**
   * Save rendered result to png file.
   */
  void save_image(string filename);

  /**
   * Wait for the scene to finish raytracing.
   */
  bool is_done();

 private:
  /**
   * Used in initialization.
   */
  bool has_valid_configuration();

  /**
   * Build acceleration structures.
   */
  void build_accel();

  /**
   * Visualize acceleration structures.
   */
  void visualize_accel() const;

  /**
   * Trace an ray in the scene.
   */
  Spectrum trace_ray(const Ray& ray);

  /**
   * Trace a camera ray given by the pixel coordinate.
   */
  Spectrum raytrace_pixel(size_t x, size_t y);

  /**
   * Raytrace a tile of the scene and update the frame buffer. Is run
   * in a worker thread.
   */
  void raytrace_tile(int tile_x, int tile_y, int tile_w, int tile_h);

  /**
   * Implementation of a ray tracer worker thread
   */
  void worker_thread();

  /**
   * Log a ray miss.
   */
  void log_ray_miss(const Ray& r);

  /**
   * Log a ray hit.
   */
  void log_ray_hit(const Ray& r, double hit_t);

  enum State {
    INIT,       ///< to be initialized
    READY,      ///< initialized ready to do stuff
    VISUALIZE,  ///< visualizing BVH accelerator aggregate
    RENDERING,  ///< started but not completed raytracing
    DONE        ///< started and completed raytracing
  };

  // Configurables //

  State state;     ///< current state
  Scene* scene;    ///< current scene
  Camera* camera;  ///< current camera

  // Integrator sampling settings //

  size_t max_ray_depth;  ///< maximum allowed ray depth (applies to all rays)
  size_t ns_aa;  ///< number of camera rays in one pixel (along one axis)
  size_t ns_area_light;  ///< number samples per area light source
  size_t ns_diff;        ///< number of samples - diffuse surfaces
  size_t ns_glsy;        ///< number of samples - glossy surfaces
  size_t ns_refr;        ///< number of samples - refractive surfaces

  // Integration state //

  vector<int> tile_samples;  ///< current sample rate for tile
  size_t num_tiles_w;        ///< number of tiles along width of the image
  size_t num_tiles_h;        ///< number of tiles along height of the image

  // Components //

  BVHAccel* bvh;                 ///< BVH accelerator aggregate
  EnvironmentLight* envLight;    ///< environment map
  Sampler2D* gridSampler;        ///< samples unit grid
  Sampler3D* hemisphereSampler;  ///< samples unit hemisphere
  HDRImageBuffer sampleBuffer;   ///< sample buffer
  ImageBuffer frameBuffer;       ///< frame buffer
  Timer timer;                   ///< performance test timer

  // Internals //

  size_t numWorkerThreads;
  size_t imageTileSize;

  bool continueRaytracing;                  ///< rendering should continue
  std::vector<std::thread*> workerThreads;  ///< pool of worker threads
  std::atomic<int> workerDoneCount;         ///< worker threads management
  WorkQueue<WorkItem> workQueue;            ///< queue of work for the workers

  // Tonemapping Controls //

  float tm_gamma;  ///< gamma
  float tm_level;  ///< exposure level
  float tm_key;    ///< key value
  float tm_wht;    ///< white point

  // Visualizer Controls //

  std::stack<BVHNode*> selectionHistory;  ///< node selection history
  std::vector<LoggedRay> rayLog;          ///< ray tracing log
  bool show_rays;                         ///< show rays from raylog
};

}  // namespace CMU462

#endif  // CMU462_RAYTRACER_H
