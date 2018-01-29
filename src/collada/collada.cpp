#include "collada.h"
#include "math.h"

#include <assert.h>
#include <map>
#include <ctime>
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <algorithm>

// For more verbose output, uncomment the line below.
#define stat(s)  // cerr << "[COLLADA Parser] " << s << endl;

using namespace std;

namespace CMU462 {
namespace Collada {

SceneInfo* ColladaParser::scene;  // pointer to output scene description

Vector3D ColladaParser::up;                       // scene up direction
Matrix4x4 ColladaParser::transform;               // current transformation
map<string, XMLElement*> ColladaParser::sources;  // URI lookup table

// Parser Helpers //

inline Spectrum spectrum_from_string(string spectrum_string) {
  Spectrum s;

  stringstream ss(spectrum_string);
  ss >> s.r;
  ss >> s.g;
  ss >> s.b;

  return s;
}

inline Color color_from_string(string color_string) {
  Color c;

  stringstream ss(color_string);
  ss >> c.r;
  ss >> c.g;
  ss >> c.b;
  ss >> c.a;

  return c;
}

void ColladaParser::uri_load(XMLElement* xml) {
  if (xml->Attribute("id")) {
    string id = xml->Attribute("id");
    sources[id] = xml;
  }

  XMLElement* child = xml->FirstChildElement();
  while (child) {
    uri_load(child);
    child = child->NextSiblingElement();
  }
}

XMLElement* ColladaParser::uri_find(string id) {
  return (sources.find(id) != sources.end()) ? sources[id] : NULL;
}

XMLElement* ColladaParser::get_element(XMLElement* xml, string query) {
  stringstream ss(query);

  // find xml element
  XMLElement* e = xml;
  string token;
  while (e && getline(ss, token, '/')) {
    e = e->FirstChildElement(token.c_str());
  }

  // handle indirection
  if (e) {
    const char* url = e->Attribute("url");
    if (url) {
      string id = url + 1;
      e = uri_find(id);
    }
  }

  return e;
}

XMLElement* ColladaParser::get_technique_common(XMLElement* xml) {
  XMLElement* common_profile = xml->FirstChildElement("profile_COMMON");
  if (common_profile) {
    XMLElement* technique = common_profile->FirstChildElement("technique");
    while (technique) {
      string sid = technique->Attribute("sid");
      if (sid == "common") return technique;
      technique = technique->NextSiblingElement("technique");
    }
  }

  return xml->FirstChildElement("technique_common");
}

XMLElement* ColladaParser::get_technique_cmu462(XMLElement* xml) {
  XMLElement* technique = get_element(xml, "extra/technique");
  while (technique) {
    string profile = technique->Attribute("profile");
    if (profile == "CMU462") return technique;
    technique = technique->NextSiblingElement("technique");
  }

  return NULL;
}

int ColladaParser::load(const char* filename, SceneInfo* sceneInfo) {
  ifstream in(filename);
  if (!in.is_open()) {
    cerr << "Warning: could not open file " << filename << endl;
    return -1;
  }
  in.close();

  XMLDocument doc;
  doc.LoadFile(filename);
  if (doc.Error()) {
    stat("XML error: ");
    doc.PrintError();
    exit(EXIT_FAILURE);
  }

  // Check XML schema
  XMLElement* root = doc.FirstChildElement("COLLADA");
  if (!root) {
    stat("Error: not a COLLADA file!") exit(EXIT_FAILURE);
  } else {
    stat("Loading COLLADA file...");
  }

  // Set output scene pointer
  scene = sceneInfo;

  // Build uri table
  uri_load(root);

  // Load assets - correct up direction
  if (XMLElement* e_asset = get_element(root, "asset")) {
    XMLElement* up_axis = get_element(e_asset, "up_axis");
    if (!up_axis) {
      stat("Error: No up direction defined in COLLADA file");
      exit(EXIT_FAILURE);
    }

    // get up direction and correct non-Y_UP scenes by setting a non-identity
    // global entry transformation, assuming right hand coordinate system for
    // both input and output

    string up_dir = up_axis->GetText();
    transform = Matrix4x4::identity();
    if (up_dir == "X_UP") {
      // swap X-Y and negate Z
      transform(0, 0) = 0;
      transform(0, 1) = 1;
      transform(1, 0) = 1;
      transform(1, 1) = 0;
      transform(2, 2) = -1;

      // local up direction for lights and cameras
      up = Vector3D(1, 0, 0);

    } else if (up_dir == "Z_UP") {
      // swap Z-Y matrix and negate X
      transform(1, 1) = 0;
      transform(1, 2) = 1;
      transform(2, 1) = 1;
      transform(2, 2) = 0;
      transform(0, 0) = -1;

      // local up direction cameras
      up = Vector3D(0, 0, 1);

    } else if (up_dir == "Y_UP") {
      up = Vector3D(0, 1, 0);  // no need to correct Y_UP as its used internally
    } else {
      stat("Error: invalid up direction in COLLADA file");
      exit(EXIT_FAILURE);
    }
  }

  // Load scene -
  // A scene should only have one visual_scene instance, this constraint
  // creates a one-to-one relationship between the document, the top-level
  // scene, and its visual description (COLLADA spec 1.4 page 91)
  if (XMLElement* e_scene = get_element(root, "scene/instance_visual_scene")) {
    stat("Loading scene...");

    // parse all nodes in scene
    XMLElement* e_node = get_element(e_scene, "node");
    while (e_node) {
      parse_node(e_node);
      e_node = e_node->NextSiblingElement("node");
    }

  } else {
    stat("Error: No scene description found in file:" << filename);
    return -1;
  }

  return 0;
}

int ColladaParser::save(const char* filename, const SceneInfo* sceneInfo) {
  // TODO: not yet supported
  return 0;
}

void ColladaParser::parse_node(XMLElement* xml) {
  // create new node
  Node node = Node();

  // name & id
  node.id = xml->Attribute("id");
  node.name = xml->Attribute("name");
  stat(" |- Node: " << node.name << " (id:" << node.id << ")");

  // node transformation -
  // combine in order of declaration if the transformations are given as a
  // transformation list
  XMLElement* e = xml->FirstChildElement();
  while (e) {
    string name = e->Name();

    // transformation - matrix
    if (name == "matrix") {
      string s = e->GetText();
      stringstream ss(s);

      Matrix4x4 mat;
      ss >> mat(0, 0);
      ss >> mat(0, 1);
      ss >> mat(0, 2);
      ss >> mat(0, 3);
      ss >> mat(1, 0);
      ss >> mat(1, 1);
      ss >> mat(1, 2);
      ss >> mat(1, 3);
      ss >> mat(2, 0);
      ss >> mat(2, 1);
      ss >> mat(2, 2);
      ss >> mat(2, 3);
      ss >> mat(3, 0);
      ss >> mat(3, 1);
      ss >> mat(3, 2);
      ss >> mat(3, 3);

      node.transform = mat;
      break;
    }

    // transformation - rotate
    if (name == "rotate") {
      Matrix4x4 m;

      string s = e->GetText();
      stringstream ss(s);

      string sid = e->Attribute("sid");
      switch (sid.back()) {
        case 'X':
          ss >> m(1, 1);
          ss >> m(1, 2);
          ss >> m(2, 1);
          ss >> m(2, 2);
          break;
        case 'Y':
          ss >> m(0, 0);
          ss >> m(2, 0);
          ss >> m(0, 2);
          ss >> m(2, 2);
          break;
        case 'Z':
          ss >> m(0, 0);
          ss >> m(0, 1);
          ss >> m(1, 0);
          ss >> m(1, 1);
          break;
        default:
          break;
      }

      node.transform = m * node.transform;
    }

    // transformation - translate
    if (name == "translate") {
      Matrix4x4 m;

      string s = e->GetText();
      stringstream ss(s);

      ss >> m(0, 3);
      ss >> m(1, 3);
      ss >> m(2, 3);

      node.transform = m * node.transform;
    }

    // transformation - scale
    if (name == "scale") {
      Matrix4x4 m;

      string s = e->GetText();
      stringstream ss(s);

      ss >> m(0, 0);
      ss >> m(1, 1);
      ss >> m(1, 1);

      node.transform = m * node.transform;
    }

    // transformation - skew
    // Note (sky): ignored for now

    // transformation - lookat
    // Note (sky): ignored for now

    e = e->NextSiblingElement();
  }

  // push transformations
  Matrix4x4 transform_save = transform;

  // combine transformations
  node.transform = transform * node.transform;
  transform = node.transform;

  // parse child nodes if node is a joint
  XMLElement* e_child = get_element(xml, "node");
  while (e_child) {
    parse_node(e_child);
    e_child = e_child->NextSiblingElement("node");
  }

  // pop transformations
  transform = transform_save;

  // node instance -
  // non-joint nodes must contain a scene object instance
  XMLElement* e_camera = get_element(xml, "instance_camera");
  XMLElement* e_light = get_element(xml, "instance_light");
  XMLElement* e_geometry = get_element(xml, "instance_geometry");

  if (e_camera) {
    CameraInfo* camera = new CameraInfo();
    parse_camera(e_camera, *camera);
    node.instance = camera;
  } else if (e_light) {
    LightInfo* light = new LightInfo();
    parse_light(e_light, *light);
    node.instance = light;
  } else if (e_geometry) {
    if (get_element(e_geometry, "mesh")) {
      // mesh geometry
      PolymeshInfo* polymesh = new PolymeshInfo();
      parse_polymesh(e_geometry, *polymesh);

      // mesh material
      XMLElement* e_instance_material = get_element(
          xml,
          "instance_geometry/bind_material/technique_common/instance_material");
      if (e_instance_material) {
        if (!e_instance_material->Attribute("target")) {
          stat(
              "Error: no target material in instance: " << e_instance_material);
          exit(EXIT_FAILURE);
        }

        string material_id = e_instance_material->Attribute("target") + 1;
        XMLElement* e_material = uri_find(material_id);
        if (!e_material) {
          stat("Error: invalid target material id : " << material_id);
          exit(EXIT_FAILURE);
        }

        MaterialInfo* material = new MaterialInfo();
        parse_material(e_material, *material);
        polymesh->material = material;
      }

      node.instance = polymesh;

    } else if (get_element(e_geometry, "extra")) {
      // sphere geometry
      SphereInfo* sphere = new SphereInfo();
      parse_sphere(e_geometry, *sphere);

      // sphere material
      XMLElement* e_instance_material = get_element(
          xml,
          "instance_geometry/bind_material/technique_common/instance_material");
      if (e_instance_material) {
        if (!e_instance_material->Attribute("target")) {
          stat(
              "Error: no target material in instance: " << e_instance_material);
          exit(EXIT_FAILURE);
        }

        string material_id = e_instance_material->Attribute("target") + 1;
        XMLElement* e_material = uri_find(material_id);
        if (!e_material) {
          stat("Error: invalid target material id : " << material_id);
          exit(EXIT_FAILURE);
        }

        MaterialInfo* material = new MaterialInfo();
        parse_material(e_material, *material);
        sphere->material = material;
      }

      node.instance = sphere;
    }
  }

  // add node to scene
  scene->nodes.push_back(node);
}

void ColladaParser::parse_camera(XMLElement* xml, CameraInfo& camera) {
  // name & id
  camera.id = xml->Attribute("id");
  camera.name = xml->Attribute("name");
  camera.type = Instance::CAMERA;

  // default look direction is down the up axis
  camera.up_dir = up;
  camera.view_dir = Vector3D(0, 0, -1);

  // NOTE (sky): only supports perspective for now
  XMLElement* e_perspective =
      get_element(xml, "optics/technique_common/perspective");
  if (e_perspective) {
    XMLElement* e_xfov = e_perspective->FirstChildElement("xfov");
    XMLElement* e_yfov = e_perspective->FirstChildElement("yfov");
    XMLElement* e_znear = e_perspective->FirstChildElement("znear");
    XMLElement* e_zfar = e_perspective->FirstChildElement("zfar");

    camera.hFov = e_xfov ? atof(e_xfov->GetText()) : 50.0f;
    camera.vFov = e_yfov ? atof(e_yfov->GetText()) : 35.0f;
    camera.nClip = e_znear ? atof(e_znear->GetText()) : 0.001f;
    camera.fClip = e_zfar ? atof(e_zfar->GetText()) : 1000.0f;

    if (!e_yfov) {  // if vfov is not given, compute from aspect ratio
      XMLElement* e_aspect_ratio = get_element(e_perspective, "aspect_ratio");
      if (e_aspect_ratio) {
        float aspect_ratio = atof(e_aspect_ratio->GetText());
        camera.vFov =
            2 * degrees(atan(tan(radians(0.5 * camera.hFov)) / aspect_ratio));
      } else {
        stat("Error: incomplete perspective definition in: " << camera.id);
        exit(EXIT_FAILURE);
      }
    }
  } else {
    stat("Error: no perspective defined in camera: " << camera.id);
    exit(EXIT_FAILURE);
  }

  // print summary
  stat("  |- " << camera);
}

void ColladaParser::parse_light(XMLElement* xml, LightInfo& light) {
  // name & id
  light.id = xml->Attribute("id");
  light.name = xml->Attribute("name");
  light.type = Instance::LIGHT;

  XMLElement* technique = NULL;
  XMLElement* technique_common = get_technique_common(xml);
  XMLElement* technique_cmu462 = get_technique_cmu462(xml);

  technique = technique_cmu462 ? technique_cmu462 : technique_common;
  if (!technique) {
    stat("Error: No supported profile defined in light: " << light.id);
    exit(EXIT_FAILURE);
  }

  // light parameters
  XMLElement* e_light = technique->FirstChildElement();
  if (e_light) {
    // type
    string type = e_light->Name();

    // type-specific parameters
    if (type == "ambient") {
      light.light_type = LightType::AMBIENT;
      XMLElement* e_color = get_element(e_light, "color");
      if (e_color) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
      } else {
        stat("Error: No color definition in ambient light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else if (type == "directional") {
      light.light_type = LightType::DIRECTIONAL;
      XMLElement* e_color = get_element(e_light, "color");
      if (e_color) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
      } else {
        stat("Error: No color definition in directional light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else if (type == "area") {
      light.light_type = LightType::AREA;
      XMLElement* e_color = get_element(e_light, "color");
      if (e_color) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
      } else {
        stat("Error: No color definition in area light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else if (type == "point") {
      light.light_type = LightType::POINT;
      XMLElement* e_color = get_element(e_light, "color");
      XMLElement* e_constant_att = get_element(e_light, "constant_attenuation");
      XMLElement* e_linear_att = get_element(e_light, "linear_attenuation");
      XMLElement* e_quadratic_att =
          get_element(e_light, "quadratic_attenuation");
      if (e_color && e_constant_att && e_linear_att && e_quadratic_att) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
        light.constant_att = atof(e_constant_att->GetText());
        light.constant_att = atof(e_linear_att->GetText());
        light.constant_att = atof(e_quadratic_att->GetText());
      } else {
        stat("Error: incomplete definition of point light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else if (type == "spot") {
      light.light_type = LightType::SPOT;
      XMLElement* e_color = get_element(e_light, "color");
      XMLElement* e_falloff_deg = e_light->FirstChildElement("falloff_angle");
      XMLElement* e_falloff_exp =
          e_light->FirstChildElement("falloff_exponent");
      XMLElement* e_constant_att = get_element(e_light, "constant_attenuation");
      XMLElement* e_linear_att = get_element(e_light, "linear_attenuation");
      XMLElement* e_quadratic_att =
          get_element(e_light, "quadratic_attenuation");
      if (e_color && e_falloff_deg && e_falloff_exp && e_constant_att &&
          e_linear_att && e_quadratic_att) {
        string color_string = e_color->GetText();
        light.spectrum = spectrum_from_string(color_string);
        light.constant_att = atof(e_falloff_deg->GetText());
        light.constant_att = atof(e_falloff_exp->GetText());
        light.constant_att = atof(e_constant_att->GetText());
        light.constant_att = atof(e_linear_att->GetText());
        light.constant_att = atof(e_quadratic_att->GetText());
      } else {
        stat("Error: incomplete definition of spot light: " << light.id);
        exit(EXIT_FAILURE);
      }
    } else {
      stat("Error: Light type " << type << " in light: " << light.id
                                << "is not supported");
      exit(EXIT_FAILURE);
    }
  }

  // print summary
  stat("  |- " << light);
}

void ColladaParser::parse_sphere(XMLElement* xml, SphereInfo& sphere) {
  // name & id
  sphere.id = xml->Attribute("id");
  sphere.name = xml->Attribute("name");
  sphere.type = Instance::SPHERE;

  XMLElement* e_technique = get_technique_cmu462(xml);
  if (!e_technique) {
    stat("Error: no 462 profile technique in geometry: " << sphere.id);
    exit(EXIT_FAILURE);
  }

  XMLElement* e_radius = get_element(e_technique, "sphere/radius");
  if (!e_radius) {
    stat("Error: invalid sphere definition in geometry: " << sphere.id);
    exit(EXIT_FAILURE);
  }

  sphere.radius = atof(e_radius->GetText());

  // print summary
  stat("  |- " << sphere);
}

void ColladaParser::parse_polymesh(XMLElement* xml, PolymeshInfo& polymesh) {
  // name & id
  polymesh.id = xml->Attribute("id");
  polymesh.name = xml->Attribute("name");
  polymesh.type = Instance::POLYMESH;


  XMLElement* e_mesh = xml->FirstChildElement("mesh");
  if (!e_mesh) {
    stat("Error: no mesh data defined in geometry: " << polymesh.id);
    exit(EXIT_FAILURE);
  }

  // array sources
  map<string, vector<float> > arr_sources;
  XMLElement* e_source = e_mesh->FirstChildElement("source");
  while (e_source) {
    // source id
    string source_id = e_source->Attribute("id");

    // source float array - other formats not handled
    XMLElement* e_float_array = e_source->FirstChildElement("float_array");
    if (e_float_array) {
      float f;
      vector<float> floats;

      // load float array string
      string s = e_float_array->GetText();
      stringstream ss(s);

      // load float array
      size_t num_floats = e_float_array->IntAttribute("count");
      for (size_t i = 0; i < num_floats; ++i) {
        ss >> f;
        floats.push_back(f);
      }

      // add to array sources
      arr_sources[source_id] = floats;
    }

    // parse next source
    e_source = e_source->NextSiblingElement("source");
  }

  // vertices
  vector<Vector3D> vertices;
  string vertices_id;
  XMLElement* e_vertices = e_mesh->FirstChildElement("vertices");
  if (!e_vertices) {
    stat("Error: no vertices defined in geometry: " << polymesh.id);
    exit(EXIT_FAILURE);
  } else {
    vertices_id = e_vertices->Attribute("id");
  }

  XMLElement* e_input = e_vertices->FirstChildElement("input");
  while (e_input) {
    // input semantic
    string semantic = e_input->Attribute("semantic");

    // semantic - position
    if (semantic == "POSITION") {
      string source = e_input->Attribute("source") + 1;
      if (arr_sources.find(source) != arr_sources.end()) {
        vector<float>& floats = arr_sources[source];
        size_t num_floats = floats.size();
        for (size_t i = 0; i < num_floats; i += 3) {
          Vector3D v = Vector3D(floats[i], floats[i + 1], floats[i + 2]);
          vertices.push_back(v);
        }
      } else {
        stat("Error: undefined input source: " << source);
        exit(EXIT_FAILURE);
      }
    }

    // NOTE (sky) : only positions are handled currently

    e_input = e_input->NextSiblingElement("input");
  }

  // polylist
  XMLElement* e_polylist;
  XMLElement* e_polylistprim = e_mesh->FirstChildElement("polylist");
  XMLElement* e_tri = e_mesh->FirstChildElement("triangles");

  bool is_polylist;

  // Supports both triangles and polylist as primitives
  if (e_polylistprim) {
    e_polylist = e_polylistprim;
    is_polylist = true;
  } else if (e_tri) {
    e_polylist = e_tri;
    is_polylist = false;
  } else {
    stat("Error: Mesh uses neither polylist nor triangles");
    exit(EXIT_FAILURE);
  }
 
  if (e_polylist) {
    // input arrays & array offsets
    bool has_vertex_array = false;
    size_t vertex_offset = 0;
    bool has_normal_array = false;
    size_t normal_offset = 0;
    bool has_texcoord_array = false;
    size_t texcoord_offset = 0;

    // input arr_sources
    XMLElement* e_input = e_polylist->FirstChildElement("input");
    while (e_input) {
      string semantic = e_input->Attribute("semantic");
      string source = e_input->Attribute("source") + 1;
      size_t offset = e_input->IntAttribute("offset");

      // vertex array source
      if (semantic == "VERTEX") {
        has_vertex_array = true;
        vertex_offset = offset;

        if (source == vertices_id) {
          polymesh.vertices.resize(vertices.size());
          copy(vertices.begin(), vertices.end(), polymesh.vertices.begin());
        } else {
          stat("Error: undefined source for VERTEX semantic: " << source);
          exit(EXIT_FAILURE);
        }
      }

      // normal array source
      if (semantic == "NORMAL") {
        has_normal_array = true;
        normal_offset = offset;

        if (arr_sources.find(source) != arr_sources.end()) {
          vector<float>& floats = arr_sources[source];
          size_t num_floats = floats.size();
          for (size_t i = 0; i < num_floats; i += 3) {
            Vector3D n = Vector3D(floats[i], floats[i + 1], floats[i + 2]);
            polymesh.normals.push_back(n);
          }
        } else {
          stat("Error: undefined source for NORMAL semantic: " << source);
          exit(EXIT_FAILURE);
        }
      }

      // texcoord array source
      if (semantic == "TEXCOORD") {
        has_texcoord_array = true;
        texcoord_offset = offset;

        if (arr_sources.find(source) != arr_sources.end()) {
          vector<float>& floats = arr_sources[source];
          size_t num_floats = floats.size();
          for (size_t i = 0; i < num_floats; i += 2) {
            Vector2D n = Vector2D(floats[i], floats[i + 1]);
            polymesh.texcoords.push_back(n);
          }
        } else {
          stat("Error: undefined source for TEXCOORD semantic: " << source);
          exit(EXIT_FAILURE);
        }
      }

      e_input = e_input->NextSiblingElement("input");
    }

    // polygon info
    size_t num_polygons = e_polylist->IntAttribute("count");
    size_t stride = (has_vertex_array ? 1 : 0) + (has_normal_array ? 1 : 0) +
                    (has_texcoord_array ? 1 : 0);

    // create polygon size array and compute size of index array
    vector<size_t> sizes;
    size_t num_indices = 0;

    if (is_polylist) {
      XMLElement* e_vcount = e_polylist->FirstChildElement("vcount");
      if (e_vcount) {
        size_t size;
        string s = e_vcount->GetText();
        stringstream ss(s);
  
        for (size_t i = 0; i < num_polygons; ++i) {
          ss >> size;
          sizes.push_back(size);
          num_indices += size * stride;
        }
  
      } else {
        stat("Error: polygon sizes undefined in geometry: " << polymesh.id);
        exit(EXIT_FAILURE);
      }
    } else {
    // Not polylist, so must be triangles
      size_t size = 3;
      for (size_t i = 0; i < num_polygons; ++i) {
        sizes.push_back(size);
        num_indices += size * stride;
      }
    }
   
    // index array
    vector<size_t> indices;
    XMLElement* e_p = e_polylist->FirstChildElement("p");
    if (e_p) {
      size_t index;
      string s = e_p->GetText();
      stringstream ss(s);

      for (size_t i = 0; i < num_indices; ++i) {
        ss >> index;
        indices.push_back(index);
      }

    } else {
      stat("Error: no index array defined in geometry: " << polymesh.id);
      exit(EXIT_FAILURE);
    }

    // create polygons
    polymesh.polygons.resize(num_polygons);

    // vertex array indices
    if (has_vertex_array) {
      size_t k = 0;
      for (size_t i = 0; i < num_polygons; ++i) {
        for (size_t j = 0; j < sizes[i]; ++j) {
          polymesh.polygons[i].vertex_indices.push_back(
              indices[k * stride + vertex_offset]);
          k++;
        }
      }
    }

    // normal array indices
    if (has_normal_array) {
      size_t k = 0;
      for (size_t i = 0; i < num_polygons; ++i) {
        for (size_t j = 0; j < sizes[i]; ++j) {
          polymesh.polygons[i].normal_indices.push_back(
              indices[k * stride + normal_offset]);
          k++;
        }
      }
    }

    // texcoord array indices
    if (has_normal_array) {
      size_t k = 0;
      for (size_t i = 0; i < num_polygons; ++i) {
        for (size_t j = 0; j < sizes[i]; ++j) {
          polymesh.polygons[i].texcoord_indices.push_back(
              indices[k * stride + texcoord_offset]);
          k++;
        }
      }
    }
  }

  // print summary
  stat("  |- " << polymesh);
}

void ColladaParser::parse_material(XMLElement* xml, MaterialInfo& material) {
  // name & id
  material.id = xml->Attribute("id");
  material.name = xml->Attribute("name");
  material.type = Instance::MATERIAL;

  // parse effect
  XMLElement* e_effect = get_element(xml, "instance_effect");
  if (e_effect) {
    // if the material does not have additional specification in the 462
    // profile. The diffuse color will be used to create a diffuse BSDF,
    // Other information from the common profile are ignored

    XMLElement* tech_common = get_technique_common(e_effect);  // common profile
    XMLElement* tech_cmu462 = get_technique_cmu462(e_effect);  // cmu462 profile

    if (tech_cmu462) {
      XMLElement* e_bsdf = tech_cmu462->FirstChildElement();
      while (e_bsdf) {
        string type = e_bsdf->Name();
        if (type == "emission") {
          XMLElement* e_radiance = get_element(e_bsdf, "radiance");
          Spectrum radiance =
              spectrum_from_string(string(e_radiance->GetText()));
          BSDF* bsdf = new EmissionBSDF(radiance);
          material.bsdf = bsdf;
        } else if (type == "mirror") {
          XMLElement* e_reflectance = get_element(e_bsdf, "reflectance");
          Spectrum reflectance =
              spectrum_from_string(string(e_reflectance->GetText()));
          BSDF* bsdf = new MirrorBSDF(reflectance);
          material.bsdf = bsdf;
          /*
          if (type == "glossy") {
            XMLElement *e_reflectance  = get_element(e_bsdf, "reflectance");
            XMLElement *e_roughness = get_element(e_bsdf, "roughness");
            Spectrum reflectance =
          spectrum_from_string(string(e_reflectance->GetText()));
            float roughness = atof(e_roughness->GetText());
            BSDF* bsdf = new GlossyBSDF(reflectance, roughness);
            material.bsdf = bsdf;
          */
        } else if (type == "refraction") {
          XMLElement* e_transmittance = get_element(e_bsdf, "transmittance");
          XMLElement* e_roughness = get_element(e_bsdf, "roughness");
          XMLElement* e_ior = get_element(e_bsdf, "ior");
          Spectrum transmittance =
              spectrum_from_string(string(e_transmittance->GetText()));
          float roughness = atof(e_roughness->GetText());
          float ior = atof(e_ior->GetText());
          BSDF* bsdf = new RefractionBSDF(transmittance, roughness, ior);
          material.bsdf = bsdf;
        } else if (type == "glass") {
          XMLElement* e_transmittance = get_element(e_bsdf, "transmittance");
          XMLElement* e_reflectance = get_element(e_bsdf, "reflectance");
          XMLElement* e_roughness = get_element(e_bsdf, "roughness");
          XMLElement* e_ior = get_element(e_bsdf, "ior");
          Spectrum transmittance =
              spectrum_from_string(string(e_transmittance->GetText()));
          Spectrum reflectance =
              spectrum_from_string(string(e_reflectance->GetText()));
          float roughness = atof(e_roughness->GetText());
          float ior = atof(e_ior->GetText());
          BSDF* bsdf =
              new GlassBSDF(transmittance, reflectance, roughness, ior);
          material.bsdf = bsdf;
        }

        e_bsdf = e_bsdf->NextSiblingElement();
      }
    } else if (tech_common) {
      XMLElement* e_diffuse = get_element(tech_common, "phong/diffuse/color");
      if (e_diffuse) {
        Spectrum albedo = spectrum_from_string(string(e_diffuse->GetText()));
        material.bsdf = new DiffuseBSDF(albedo);
      } else {
        material.bsdf = new DiffuseBSDF(Spectrum(.5f, .5f, .5f));
      }
    } else {
      BSDF* bsdf = new DiffuseBSDF(Spectrum(.5f, .5f, .5f));
      material.bsdf = bsdf;
    }
  } else {
    stat("Error: no target effects found for material: " << material.id);
    exit(EXIT_FAILURE);
  }

  // print summary
  stat("  |- " << material);
}

// ====================================================================
// ============ ColladaWriter =========================================
// ====================================================================

bool ColladaWriter::writeScene(DynamicScene::Scene& scene,
                               const char* filename) {
  ofstream out(filename);
  if (!out.is_open()) {
    cerr << "WARNING: Could not open file " << filename
         << " for COLLADA export!" << endl;
    return false;
  }

  writeHeader(out);
  writeGeometry(out, scene);
  // TODO lights, camera, materials
  writeVisualScenes(out, scene);
  writeFooter(out);

  return true;
}

void writeCurrentTime(ofstream& out) {
  auto t = time(nullptr);
  auto tm = *localtime(&t);

  out << tm.tm_year + 1900 << "-";
  if (tm.tm_mon < 10) out << "0";
  out << tm.tm_mon + 1 << "-";
  if (tm.tm_mday < 10) out << "0";
  out << tm.tm_mday << "T";
  if (tm.tm_hour < 10) out << "0";
  out << tm.tm_hour << ":";
  if (tm.tm_min < 10) out << "0";
  out << tm.tm_min << ":";
  if (tm.tm_sec < 10) out << "0";
  out << tm.tm_sec;
}

void ColladaWriter::writeHeader(ofstream& out) {
  out << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl;
  out << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" "
         "version=\"1.4.1\">"
      << endl;
  out << "<asset>" << endl;
  out << "   <contributor>" << endl;
  out << "      <author>Scotty</author>" << endl;
  out << "      <authoring_tool>CMU Scotty3D (version "
         "15-462/662)</authoring_tool>"
      << endl;
  out << "   </contributor>" << endl;
  out << "   <created>";
  writeCurrentTime(out);
  out << "</created>" << endl;
  out << "   <modified>";
  writeCurrentTime(out);
  out << "</modified>" << endl;
  out << "   <unit name=\"meter\" meter=\"1\"/>" << endl;
  out << "   <up_axis>Y_UP</up_axis>" << endl;
  out << "</asset>" << endl;
}

void ColladaWriter::writeFooter(ofstream& out) { out << "</COLLADA>" << endl; }

void ColladaWriter::writeGeometry(ofstream& out, DynamicScene::Scene& scene) {
  int nMeshes = 0;

  out << "   <library_geometries>" << endl;
  for (auto o : scene.objects) {
    DynamicScene::Mesh* mesh = dynamic_cast<DynamicScene::Mesh*>(o);
    if (mesh) {
      nMeshes++;
      writeMesh(out, mesh, nMeshes);
    }
  }
  out << "   </library_geometries>" << endl;
}

void ColladaWriter::writeMesh(ofstream& out, DynamicScene::Mesh* mesh, int id) {
  HalfedgeMesh& m(mesh->mesh);
  int nV = m.nVertices();
  int nF = m.nFaces();

  // TODO transformations are currently ignored

  // assign a unique ID to each vertex (we will need these so that
  // each polygon can reference its vertices)
  int index = 0;
  for (VertexIter v = m.verticesBegin(); v != m.verticesEnd(); v++) {
    v->index = index;
    index++;
  }

  out << "      <geometry id=\"M" << id << "\" name=\"Mesh" << id << "\">"
      << endl;
  out << "         <mesh>" << endl;

  // positions -------------
  out << "            <source id=\"M" << id << "-positions\">" << endl;
  out << "               <float_array id=\"M" << id
      << "-positions-array\" count=\"" << 3 * nV << "\">" << endl;
  for (VertexIter v = m.verticesBegin(); v != m.verticesEnd(); v++) {
    Vector3D p = v->position;
    out << "                  ";
    out << p.x << " " << p.y << " " << p.z << endl;
  }
  out << "               </float_array>" << endl;
  out << "               <technique_common>" << endl;
  out << "                  <accessor source=\"#M" << id
      << "-positions-array\" count=\"" << nV << "\" stride=\"3\">" << endl;
  out << "                     <param name=\"X\" type=\"float\"/>" << endl;
  out << "                     <param name=\"Y\" type=\"float\"/>" << endl;
  out << "                     <param name=\"Z\" type=\"float\"/>" << endl;
  out << "                  </accessor>" << endl;
  out << "               </technique_common>" << endl;
  out << "            </source>" << endl;

  // vertices -------------
  out << "            <vertices id=\"M" << id << "-vertices\">" << endl;
  out << "               <input semantic=\"POSITION\" source=\"#M" << id
      << "-positions\"/>" << endl;
  out << "            </vertices>" << endl;

  // polygons -------------
  out << "         <polylist count=\"" << nF << "\">" << endl;
  out << "            <input semantic=\"VERTEX\" source=\"#M" << id
      << "-vertices\" offset=\"0\"/>" << endl;
  out << "            <vcount>";
  for (FaceIter f = m.facesBegin(); f != m.facesEnd(); f++) {
    out << f->degree() << " ";
  }
  out << "            </vcount>" << endl;
  out << "            <p>" << endl;
  for (FaceIter f = m.facesBegin(); f != m.facesEnd(); f++) {
    out << "               ";
    HalfedgeIter h = f->halfedge();
    do {
      out << h->vertex()->index << " ";
      h = h->next();
    } while (h != f->halfedge());
    out << endl;
  }
  out << "            </p>" << endl;
  out << "         </polylist>" << endl;

  out << "         </mesh>" << endl;
  out << "      </geometry>" << endl;
}

void ColladaWriter::writeVisualScenes(ofstream& out,
                                      DynamicScene::Scene& scene) {
  out << "   <library_visual_scenes>" << endl;
  out << "      <visual_scene id=\"ScottyScene\">" << endl;

  int nMeshes = 0;
  int nNodes = 0;
  for (auto o : scene.objects) {
    DynamicScene::Mesh* mesh = dynamic_cast<DynamicScene::Mesh*>(o);
    if (mesh) {
      nMeshes++;
      nNodes++;
      out << "         <node id=\"N" << nNodes << "\" name=\"Node" << nNodes
          << "\">" << endl;
      out << "            <instance_geometry url=\"#M" << nMeshes << "\">"
          << endl;
      out << "            </instance_geometry>" << endl;
      out << "         </node>" << endl;
    }
  }

  out << "      </visual_scene>" << endl;
  out << "   </library_visual_scenes>" << endl;

  out << "   <scene>" << endl;
  out << "      <instance_visual_scene url=\"#ScottyScene\"/>" << endl;
  out << "   </scene>" << endl;
}

}  // namespace Collada
}  // namespace CMU462
