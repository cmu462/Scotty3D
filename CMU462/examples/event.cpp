#include <string>
#include <iostream>

#include "CMU462/viewer.h"
#include "CMU462/renderer.h"

using namespace std;
using namespace CMU462;

class EventDisply : public Renderer {
 public:

  ~EventDisply() { }

  string name() {
    return "Event handling example";
  }

  string info() {
    return "Event handling example";
  }

  void init() {

    text_mgr.init(use_hdpi);

    size = 16;
    line0 = text_mgr.add_line(0.0, 0.0, "Hi there!", size, Color::White);

    return;
  }

  void render() {

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
      text_mgr.set_anchor(line0, 2 * (x - .5 * w) / w, 2 * (.5 * h - y) / h);      
    }
  }

  void scroll_event(float offset_x, float offset_y) {
    size += int(offset_y + offset_x);
    text_mgr.set_size(line0, size);
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

  // my line's font size
  size_t size;

  // frame buffer size
  size_t w, h;

  // key states
  bool left_down;

};


int main( int argc, char** argv ) {

  // create viewer
  Viewer viewer = Viewer();

  // defined a user space renderer
  Renderer* renderer = new EventDisply();

  // set user space renderer
  viewer.set_renderer(renderer);

  // start the viewer
  viewer.init();
  viewer.start();

  return 0;
}
