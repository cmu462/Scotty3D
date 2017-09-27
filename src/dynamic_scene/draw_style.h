#ifndef CMU462_DYNAMICSCENE_DRAWSTYLE_H
#define CMU462_DYNAMICSCENE_DRAWSTYLE_H

#include "scene.h"

namespace CMU462 {
namespace DynamicScene {

/**
 * Used in rendering to specify how to draw the faces/meshes/lines/etc.
 */
class DrawStyle {
 public:
  void style_reset() const {
    glLineWidth(1);
    glPointSize(1);
  }

  void style_face() const { glColor4fv(&faceColor.r); }

  void style_edge() const {
    glColor4fv(&edgeColor.r);
    glLineWidth(strokeWidth);
  }

  void style_halfedge() const {
    glColor4fv(&halfedgeColor.r);
    glLineWidth(strokeWidth);
  }

  void style_vertex() const {
    glColor4fv(&vertexColor.r);
    glPointSize(vertexRadius);
  }

  void style_joint() const {
    glColor4fv(&jointColor.r);
    glLineWidth(strokeWidth);
  }

  Color halfedgeColor;
  Color vertexColor;
  Color edgeColor;
  Color faceColor;
  Color jointColor;

  float strokeWidth;
  float vertexRadius;
};

}  // namespace DynamicScene
}  // namespace CMU462

#endif  // CMU462_DYNAMICSCENE_DRAWSTYLE_H
