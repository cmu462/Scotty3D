#include "CMU462/CMU462.h"
#include "CMU462/viewer.h"

#define TINYEXR_IMPLEMENTATION
#include "CMU462/tinyexr.h"

#include "application.h"
#include "image.h"

#include <iostream>

#ifndef gid_t
typedef unsigned int gid_t;  // XXX Needed on some platforms, since gid_t is
                             // used in unistd.h but not always defined!
                             // (WARNING: May not be the right size!!)
#endif

#include "getopt.h"

using namespace std;
using namespace CMU462;

#define msg(s) cerr << "[Scotty3D] " << s << endl;

void usage(const char* binaryName) {
  printf("Usage: %s [options] <scenefile>\n", binaryName);
  printf("Program Options:\n");
  printf("  -s  <INT>        Number of camera rays per pixel\n");
  printf("  -l  <INT>        Number of samples per area light\n");
  printf("  -t  <INT>        Number of render threads\n");
  printf("  -m  <INT>        Maximum ray depth\n");
  printf("  -e  <PATH>       Path to environment map\n");
  printf("  -w  <PATH>       Run Pathtracer without GUI, save render to PATH\n");
  printf("  -h               Print this help message\n");
  printf("\n");
}

HDRImageBuffer* load_exr(const char* file_path) {
  const char* err;

  EXRImage exr;
  InitEXRImage(&exr);

  int ret = ParseMultiChannelEXRHeaderFromFile(&exr, file_path, &err);
  if (ret != 0) {
    msg("Error parsing OpenEXR file: " << err);
    return NULL;
  }

  for (int i = 0; i < exr.num_channels; i++) {
    if (exr.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
      exr.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
    }
  }

  ret = LoadMultiChannelEXRFromFile(&exr, file_path, &err);
  if (ret != 0) {
    msg("Error loading OpenEXR file: " << err);
    exit(EXIT_FAILURE);
  }

  HDRImageBuffer* envmap = new HDRImageBuffer();
  envmap->resize(exr.width, exr.height);
  float* channel_r = (float*)exr.images[2];
  float* channel_g = (float*)exr.images[1];
  float* channel_b = (float*)exr.images[0];
  for (size_t i = 0; i < exr.width * exr.height; i++) {
    envmap->data[i] = Spectrum(channel_r[i], channel_g[i], channel_b[i]);
  }

  return envmap;
}

int main(int argc, char** argv) {
  // get the options
  AppConfig config;
  int opt;
  while ((opt = getopt(argc, argv, "s:l:t:m:e:w:h")) !=
         -1) {  // for each option...
    switch (opt) {
      case 's':
        config.pathtracer_ns_aa = atoi(optarg);
        break;
      case 'l':
        config.pathtracer_ns_area_light = atoi(optarg);
        break;
      case 't':
        config.pathtracer_num_threads = atoi(optarg);
        break;
      case 'm':
        config.pathtracer_max_ray_depth = atoi(optarg);
        break;
      case 'e':
        config.pathtracer_envmap = load_exr(optarg);
        break;
      case 'w':
        if(optarg != nullptr) {
          config.pathtracer_result_path = optarg;
        }
        break;
      default:
        usage(argv[0]);
        return 1;
    }
  }

  // print usage if no argument given
  if (optind >= argc) {
    usage(argv[0]);
    return 1;
  }

  string sceneFilePath = argv[optind];
  msg("Input scene file: " << sceneFilePath);

  // parse scene
  Collada::SceneInfo* sceneInfo = new Collada::SceneInfo();
  if (Collada::ColladaParser::load(sceneFilePath.c_str(), sceneInfo) < 0) {
    msg("Error: parsing failed!");
    delete sceneInfo;
    exit(0);
  }

  // create viewer
  Viewer viewer = Viewer();

  // create application
  Application app(config);

  // set renderer
  viewer.set_renderer(&app);

  // init viewer
  viewer.init();

  // load scene
  app.load(sceneInfo);

  delete sceneInfo;

  // NOTE (sky): are we copying everything to dynamic scene? If so:
  // TODO (sky): check and make sure the destructor is freeing everything

  // Run in terminal mode if requested
  if(config.pathtracer_result_path != "") {
    app.render_scene(config.pathtracer_result_path);
    exit(EXIT_SUCCESS);
  }

  // start viewer
  viewer.start();

  // TODO:
  // apparently the meshEdit renderer instance was not destroyed properly
  // not sure if this is due to the recent refactor but if anyone got some
  // free time, check the destructor for Application.
  exit(EXIT_SUCCESS);  // shamelessly faking it

  return 0;
}
