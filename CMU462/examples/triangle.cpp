#include <string>
#include <iostream>

#include "CMU462/viewer.h"
#include "CMU462/renderer.h"

using namespace std;
using namespace CMU462;

class TriangleDrawer : public Renderer {
 public:

  ~TriangleDrawer() { }

  string name() {
    return "Drawing example";
  }

  string info() {
    return "Drawing example";
  }

  void init() {
    return;
  }
  
  void render() {
    if (shoud_draw) {
      glBegin(GL_TRIANGLES);
      glColor3f( 0.1, 0.2, 0.3);
      glVertex3f(0.0, 0.5, 0.0);
      glVertex3f(-.5, -.5, 0.0);
      glVertex3f(0.5, -.5, 0.0);
      glEnd();
    }
  }

  void resize(size_t w, size_t h) {
    
    this->w = w;
    this->h = h;

    return;
  }

  void keyboard_event(int key, int event, unsigned char mods) {
    if (key == 'R') shoud_draw = !shoud_draw; 
    return;
  }
  
 private:

  // show draw triangle
  bool shoud_draw;
  
  // frame buffer size
  size_t w, h; 

};

int main( int argc, char** argv ) {

  // create viewer
  Viewer viewer = Viewer();

  // defined a user space renderer
  Renderer* renderer = new TriangleDrawer();

  // set user space renderer
  viewer.set_renderer(renderer);

  // start the viewer
  viewer.init();
  viewer.start();

  return 0;
}

