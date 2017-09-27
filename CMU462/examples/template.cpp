#include <string>
#include <iostream>

#include "CMU462/CMU462.h"
#include "CMU462/vector3D.h"
#include "CMU462/viewer.h"
#include "CMU462/renderer.h"
#include "CMU462/misc.h"

#include "GL/glew.h"

using namespace std;
using namespace CMU462;

class Camera {
 public:

  Camera() {
    r = 5;
    phi   = PI / 4;
    theta = PI / 4;
    dir = -Vector3D(sin(phi),cos(theta),cos(phi)).unit();
    up = cross(dir,cross(Vector3D(0,1,0),dir)).unit();
    pos = -dir * r;
  }

  void set_projection() {
    Vector3D obj = pos + dir * r;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0, 1.0, 0.01f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(pos.x, pos.y, pos.z,
              obj.x, obj.y, obj.z,
               up.x,  up.y,  up.z);
  }

  Vector3D pos;
  Vector3D dir;
  Vector3D up;
  float r;
  float phi;
  float theta;
};

void draw_coordinates() {

  glBegin(GL_LINES);
  glColor3f(1.0f, 0.0f, 0.0f);
  glVertex3i(0,0,0); 
  glVertex3i(1,0,0);
   
  glColor3f(0.0f, 1.0f, 0.0f);
  glVertex3i(0,0,0); 
  glVertex3i(0,1,0); 

  glColor3f(0.0f, 0.0f, 1.0f);
  glVertex3i(0,0,0); 
  glVertex3i(0,0,1); 

  glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
  for (int x = 0; x <= 8; ++x) {
    glVertex3f(x - 4, 0, -4);
    glVertex3f(x - 4, 0,  4);
  }
  for (int z = 0; z <= 8; ++z) {
    glVertex3f(-4, 0, z - 4);
    glVertex3f( 4, 0, z - 4);
  }

  glEnd();
}

class Template : public Renderer {
 public:

  ~Template() { }

  string name() {
    return "Template Renderer";
  }

  string info() {
    return "Template Renderer";
  }

  void init() {

    text_mgr.init(use_hdpi);

    line0 = text_mgr.add_line(0.0, 0.0, "Hi there!", 16, Color::White);

    return;
  }

  void render() {

    camera.set_projection();
    draw_coordinates();
    text_mgr.render();

  }

  void resize(size_t w, size_t h) {

    this->w = w;
    this->h = h;

    text_mgr.resize(w,h);

    return;
  }

  void cursor_event(float x, float y) {
    if (left_down) {
      camera.phi += (x - mouse_x) / w * PI;
      camera.theta -= (y - mouse_y) / w * PI;
      camera.dir = -Vector3D(sin(camera.phi),cos(camera.theta),cos(camera.phi)).unit();
      camera.up = cross(camera.dir,cross(Vector3D(0,1,0), camera.dir)).unit();
      camera.pos = -camera.dir * camera.r;
    }
    mouse_x = x;
    mouse_y = y;
    float anchor_x = 2 * (x + 10 - .5 * w) / w;
    float anchor_y = 2 * (.5 * h - y + 10) / h;
    text_mgr.set_anchor(line0, anchor_x, anchor_y);
  }

  void scroll_event(float offset_x, float offset_y) {
    camera.r += (offset_x + offset_y) * camera.r * 0.1;
    camera.dir = -Vector3D(sin(camera.phi),cos(camera.theta),cos(camera.phi)).unit();
    camera.up = cross(camera.dir,cross(Vector3D(0,1,0), camera.dir)).unit();
    camera.pos = -camera.dir * camera.r;
  }

  void mouse_event(int key, int event, unsigned char mods) {
    if (key == MOUSE_LEFT) {
      if (event == EVENT_PRESS) left_down = true;
      if (event == EVENT_RELEASE) left_down = false;
    }
  }

  void keyboard_event(int key, int event, unsigned char mods) {

    string s;
    switch (event) {
      case EVENT_PRESS:    
        s = "You just pressed: ";
        break;
      case EVENT_RELEASE:
        s = "You just released: ";
        break;
      case EVENT_REPEAT:
        s = "You are holding: ";
        break;
    }

    if (key == KEYBOARD_ENTER) {
      s += "Enter";
    } else {
      char c = key;
      s += c;      
    }

    text_mgr.set_text(line0, s);
  }

 private:

  // OSD text manager
  OSDText text_mgr;

  // my line id's
  int line0;

  // frame buffer size
  size_t w, h;

  // key states
  bool left_down;

  // camera
  Camera camera;

  // mouse location
  float mouse_x;
  float mouse_y;

};

int main( int argc, char** argv ) {

  // create viewer
  Viewer viewer = Viewer();

  // defined a user space renderer
  Renderer* renderer = new Template();

  // set user space renderer
  viewer.set_renderer(renderer);

  // start the viewer
  viewer.init();
  viewer.start();

  return 0;
}
