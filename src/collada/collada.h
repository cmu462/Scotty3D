#ifndef CMU462_COLLADA_COLLADA_H
#define CMU462_COLLADA_COLLADA_H

#include <map>
#include <string>
#include <fstream>

#include "CMU462/CMU462.h"
#include "CMU462/tinyexr.h"
#include "CMU462/tinyxml2.h"

#include "camera_info.h"
#include "collada_info.h"
#include "light_info.h"
#include "sphere_info.h"
#include "polymesh_info.h"
#include "material_info.h"
#include "../dynamic_scene/scene.h"
#include "../dynamic_scene/mesh.h"

using namespace tinyxml2;

using std::vector;

namespace CMU462 {
namespace Collada {

/*
  Parses an XML file into a SceneInfo object.
*/
class ColladaParser {
 public:
  static int load(const char* filename, SceneInfo* sceneInfo);
  static int save(const char* filename, const SceneInfo* sceneInfo);

 private:
  // Pointer to the output scene description
  static SceneInfo* scene;

  // Up direction used in scene discription (set on laod) //
  static Vector3D up;

  // Current transformation - used to flatten the transformation stack
  static Matrix4x4 transform;

  // Lookup table used to handle URI indirections in COLLADA file
  // The lookup table is constructed when the file is loaded
  static std::map<std::string, XMLElement*> sources;

  // Load Collada elements with UUID into lookup table
  static void uri_load(XMLElement* xml);

  // Find xml entry point of given Collada UUID
  static XMLElement* uri_find(std::string id);

  // Get entry point to a Collada object under the xml hierarchy given by the
  // query string from the input xml node, handle indirection if needed
  static XMLElement* get_element(XMLElement* xml, std::string query);

  // Get entry point to the common profile section for the Collada object with
  // the given xml entry point
  static XMLElement* get_technique_common(XMLElement* xml);

  // Get entry point to the 462 profile section for the Collada object with
  // the given xml entry point
  static XMLElement* get_technique_cmu462(XMLElement* xml);

  static void parse_node(XMLElement* xml);
  static void parse_camera(XMLElement* xml, CameraInfo& camera);
  static void parse_light(XMLElement* xml, LightInfo& light);
  static void parse_sphere(XMLElement* xml, SphereInfo& sphere);
  static void parse_polymesh(XMLElement* xml, PolymeshInfo& polymesh);
  static void parse_material(XMLElement* xml, MaterialInfo& material);

};  // class ColladaParser

/*
  Stores a dynamic scene in a COLLADA file.
*/
class ColladaWriter {
 public:
  static bool writeScene(DynamicScene::Scene& scene, const char* filename);
  static void writeHeader(ofstream& out);
  static void writeFooter(ofstream& out);
  static void writeGeometry(ofstream& out, DynamicScene::Scene& scene);
  static void writeMesh(ofstream& out, DynamicScene::Mesh* mesh, int id);
  static void writeVisualScenes(ofstream& out, DynamicScene::Scene& scene);
};

}  // namespace Collada
}  // namespace CMU462

#endif  // CMU462_COLLADA_COLLADA_H
