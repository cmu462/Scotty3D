#ifndef TIMELINE_H
#define TIMELINE_H

/*
 * Timeline class. Written by Bryce Summers on October 29, 2015.
 *
 * Purpose : This class represents an integer timeline with important times.
 *
 * - The timeline goes from 0 to a max_time_value.
 * - The timeline has a graphic user interface consisting of several buttons.
 *
 * - When the timeline is playing, it will advance by 1 timestep at every 'step'
 * call.
 * - Important times made be arbitrarily added and removed. (a.k.a 'marked
 * times')
 *
 *
 * Usage: Timelines represent an integer based set of points.
 *        The current time should be scaled by the maximum time
 *        and scalar values to represent times at a desired timescale.
 *
 */

#include <set>
#include "GL/glew.h"

using namespace std;

namespace CMU462 {

class Timeline {
 public:
  Timeline() : x(0), y(0), w(64 * 6), h(0) { isLooping = false; }

  Timeline(int x, int y, int width, int height)
      : x(x), y(y), w(width), h(height) {
    current_frame = 0;
    max_frame = 1;
    isPlaying = false;
    isLooping = false;
  }

  ~Timeline() {}

  // Draws this timeline's buttons.
  // Draws the timeline, including the set of important times scaled to
  // interpolate times from 0 to the maximum_time.
  void draw();  // Use IMGUI???

  // Returns true iff the given mouse location is within the bounds of this
  // timeline.
  // This function sets the particular button containing (x, y) to the hover
  // state.
  bool mouse_over(float x, float y);
  bool mouse_click(float x, float y);

  bool mouse_over_timeline(float x, float y);

  bool mouse_not_in_bounds(float x, float y);

  // Returns the current time.
  int getCurrentFrame();
  int getMaxFrame();
  bool step();  // Update function?

  // Sets the ending time of this timeline.
  // REQUIRES : max_frame > 0.
  void setMaxFrame(int max_frame);

  // Adds the given time to this timeline's important times set.
  // Returns true iff the given time was not already marked.
  bool markTime(int time);

  // Removes the given time from this timeline's important times set.
  // Returns true iff the given time was alread marked.
  bool unmarkTime(int time);

  // Returns true iff the present time is marked.
  bool isPresentTimeMarked();

  // Returns the timestep of the next marked time.
  // Returns -1 if there are no more remaining important times.
  // This may be used for spline interpolation...
  int nextImportantTime();

  // Returns the timestep of the last important time that has already been
  // passed.
  // This may be used for spline interpolation...

  int previousImportantTime();

  void resize(int w, int h);
  void move(int x, int y);

  // Button actions, mimic the behavior of the buttons.
  void action_rewind();
  void action_goto_end();
  void action_stop();
  void action_play();
  void action_step_forward(int nFrames = 1);
  void action_step_backward(int nFrames = 1);
  void action_loop();
  void action_goto_next_key_frame();
  void action_goto_prev_key_frame();

  // Plays the timeline if it is stopped currently,
  // Otherwise pauses the timeline.
  void action_toggle_playing();

  bool isCurrentlyPlaying();

  // make the timeline longer or shorter by the given number of frames
  // (note that if there are events past the end of the timeline, they will
  // remain!)
  void makeLonger(int number);
  void makeShorter(int number);

  // Helpful functions for drawing simple goemetric shapes to the screen.
  // REQUIRES : A 2D Ortho projection should already be in effect.
  void drawTriangle(double x1, double y1, double x2, double y2, double x3,
                    double y3);

  void drawRectangle(double x1, double y1, double x2, double y2);
  void drawCircle(double cx, double cy, double radius, int num_segments);
  void drawLine(double x1, double y1, double x2, double y2);

  // Helper functions used to setup and tear down 2D drawing mode.
  void enter_2D_GL_draw_mode();
  void exit_2D_GL_draw_mode();

 private:
  int x, y, w, h;

  // A set of important times.
  set<int> times;

  // The timeline goes from time [0 to max_frame);
  int current_frame;
  int max_frame;

  bool isPlaying;
  bool isLooping;

  // this stores which button the user is currently hovering over.
  int hover_button_index = -1;
  int selected_button_index = -1;

  // WARNING: Don't change the values REWIND - LOOP.
  enum e_Element {
    REWIND = 0,
    STOP = 1,
    PLAY = 2,
    STEP_BACK = 3,
    STEP_FORWARD = 4,
    LOOP = 5,
    TIMELINE = 6,
    NONE = 7
  };

  e_Element getElement(int x, int y);

  int getButtonStartX();
  int getButtonW();

  void drawButton(int element_type, double x, double y, double size);
};
}

#endif  // TIMELINE_H
