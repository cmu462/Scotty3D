#ifndef CMU462_DYNAMICSCENE_MESH_H
#define CMU462_DYNAMICSCENE_MESH_H

#include "scene.h"

#include "../collada/polymesh_info.h"
#include "../halfEdgeMesh.h"
#include "../meshEdit.h"
#include "skeleton.h"

#include <map>

namespace CMU462 {
namespace DynamicScene {

// A structure for holding linear blend skinning information
class LBSInfo {
 public:
  Vector3D blendPos;
  double distance;
};

enum RenderMask {
  VERTEX = 1,
  EDGE = 2,
  FACE = 4,
  ALL = VERTEX | EDGE | FACE
};

class Mesh : public SceneObject {
 public:
  Mesh(Collada::PolymeshInfo &polyMesh, const Matrix4x4 &transform);

  ~Mesh();

  void set_draw_styles(DrawStyle *defaultStyle, DrawStyle *hoveredStyle,
                       DrawStyle *selectedStyle) override;
  virtual void draw() override;
  virtual void drawGhost() override;

  void draw_pretty() override;

  StaticScene::SceneObject *get_transformed_static_object(double t) override;

  BBox get_bbox() override;

  virtual Info getInfo() override;

  void bevelComputeNewPositions(double inset, double shift);

  virtual void drag(double x, double y, double dx, double dy,
                    const Matrix4x4 &modelViewProj) override;

  BSDF *get_bsdf();
  StaticScene::SceneObject *get_static_object() override;

  void collapse_selected_element();
  void flip_selected_edge();
  void split_selected_edge();
  void erase_selected_element();
  void bevel_selected_element();
  void upsample();
  void downsample();
  void resample();
  void triangulate();

  HalfedgeMesh mesh;

  Skeleton *skeleton;  // skeleton for mesh
  void linearBlendSkinning(bool useCapsuleRadius);
  void forward_euler(float timestep, float damping_factor);
  void symplectic_euler(float timestep, float damping_factor);
  void resetWave();
  void keyframe(double t);
  void unkeyframe(double t);

  /**
   * Rather than drawing the object geometry for display, this method draws the
   * object with unique colors that can be used to determine which object was
   * selected or "picked" by the cursor.  The parameter pickID is the lowest
   * consecutive integer that has so far not been used by any other object as
   * a picking ID.  (Draw colors are then derived from these IDs.)  This data
   * will be used by Scene::update_selection to make the final determination
   * of which object (and possibly element within that object) was picked.
   */
  virtual void draw_pick(int &pickID, bool transformed = false) override;

  /** Assigns attributes of the selection based on the ID of the
   * object that was picked.  Can assume that pickID was one of
   * the IDs generated during this object's call to draw_pick().
   */
  virtual void setSelection(int pickID, Selection &selection) override;

  /** If true, then all meshes rendered will have their normals flipped.
   * Useful for rendering backfaces.
   */
  static bool flip_normals;
  /** For all meshes rendered, masks out if we want to render verts,
   * faces, or edges (or some combination).
   */
  static RenderMask global_render_mask;

 private:
  // Helpers for draw().
  void draw_faces(bool smooth = false) const;
  void draw_edges() const;
  void draw_feature_if_needed(Selection *s) const;
  void draw_vertex(const Vertex *v) const;
  void draw_halfedge_arrow(const Halfedge *h) const;
  DrawStyle *get_draw_style(const HalfedgeElement *element) const;

  void check_finite_positions();
  bool alreadyCheckingPositions;

  // a vector of halfedges whose vertices are newly created with bevel
  // on scroll, reposition vertices referenced from these halfedges
  vector<HalfedgeIter> bevelVertices;
  // original position of beveled vertex
  Vector3D beveledVertexPos;
  // original positions of beveled edge, corresponding to bevelVertices
  vector<Vector3D> beveledEdgePos;
  // original vertex positions for face currently being beveled
  vector<Vector3D> beveledFacePos;

  DrawStyle *defaultStyle, *hoveredStyle, *selectedStyle;

  MeshResampler resampler;

  // map from picking IDs to mesh elements, generated during draw_pick
  // and used by setSelection
  std::map<int, HalfedgeElement *> idToElement;

  // Assigns the next consecutive pickID to the given element, and
  // sets the GL color accordingly.  Also increments pickID.
  void newPickElement(int &pickID, HalfedgeElement *e);

  // material
  BSDF *bsdf;

};

}  // namespace DynamicScene
}  // namespace CMU462

#endif  // CMU462_DYNAMICSCENE_MESH_H
