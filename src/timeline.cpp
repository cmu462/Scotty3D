/*
 * Time Line Implementation.
 *
 * Started on October 29th, 2015 by Bryce Summers.
 *
 */

#include "timeline.h"
#include <iostream>
#include <math.h>

using namespace std;

namespace CMU462 {

// Draws this timeline's buttons.
// Draws the timeline, including the set of important times scaled to
// interpolate times from 0 to the maximum_time.
// ASSUMES that Opengl 2D Drawing Transforms are currently on the stack.
void Timeline::draw() {
  // -- White with opacity .8;
  glColor4f(1.0, 1.0, 1.0, 0.8);

  drawRectangle(x, y, x + w, y + h);

  double button_start_x = getButtonStartX();
  double border_size = 2;

  for (int i = 0; i < 6; i++) {
    // Draw each button.
    // Draw a button highlighted if hover_button_index == i.
    double b_x = button_start_x + i * h + border_size;
    double b_y = y + border_size;
    double size = h - border_size * 2;

    // Backgrounds for each button.
    // Black with opacity .3
    glColor4f(0.0, 0.0, 0.0, 0.3);
    drawRectangle(b_x, b_y, b_x + size, b_y + size);

    // Boolean button is enabled.
    if (i == LOOP && isLooping) {
      glColor4f(0.0, 0.0, 1.0, 1.0);
    }
    else if (selected_button_index == i) {
      glColor4f(0.0, 0.0, 1.0, 1.0);
    }
    else if (hover_button_index == i) {
      glColor4f(1.0, 0.0, 0.0, 1.0);
    }
    // Default, no hover, not enabled.
    else {
      // Faded red.
      glColor4f(0.3, 0.0, 0.0, 0.8);
    }

    drawButton(i, b_x, b_y, size);
  }

  // -- Draw the timeline with its various key frame locations.

  // First draw the timeline background.
  // Black with opacity .3
  glColor4f(0.0, 0.0, 0.0, 0.3);

  // Key frame time indicator color, (BLUE).
  glColor4f(0.0, 0.0, 1.0, .7);

  double line_w = button_start_x - x;

  for (set<int>::iterator iter = times.begin(); iter != times.end(); iter++) {
    // Draw a rectangle at x + (*iter)/max_frame * w to indicate keyframe times
    double t_x = x + (*iter) * 1.0 / max_frame * line_w;
    drawRectangle(t_x, y + border_size * 2, t_x + 1, y + h - border_size * 2);
  }

  glColor4f(1.0, 0.0, 0.0, 1.0);

  double current_x = x + current_frame * 1.0 / max_frame * line_w;
  drawRectangle(current_x - 2, y + border_size, current_x, y + h - border_size);
}

int Timeline::getButtonStartX() { return this->x + w - h * 6; }

// A buttons width is equal to the height of this timeline.
int Timeline::getButtonW() { return h; }

void Timeline::drawButton(int element_type, double x, double y, double size) {
  double border_size = size / 10.0;
  double x_left = x + border_size;
  double x_right = x + size - border_size;
  double y_up = y + border_size;
  double y_down = y + size - border_size;

  double x_mid = (x_left + x_right) / 2.0;
  double y_mid = (y_up + y_down) / 2.0;

  double x_mid_l = x_mid - border_size / 2;
  double x_mid_r = x_mid + border_size / 2;

  switch (element_type) {
    case REWIND:

      drawTriangle(x_left, y_mid, x_mid_l, y_up, x_mid_l, y_down);

      drawTriangle(x_mid_r, y_mid, x_right, y_up, x_right, y_down);

      break;

    case STOP:

      drawRectangle(x_left, y_up, x_right, y_down);

      break;

    case PLAY:

      drawTriangle(x_left, y_up, x_right, y_mid, x_left, y_down);
      break;

    case STEP_BACK:

      drawTriangle(x_left, y_mid, x_mid_l, y_up, x_mid_l, y_down);

      drawRectangle(x_mid_r, y_up, x_right, y_down);
      break;

    case STEP_FORWARD:

      drawRectangle(x_left, y_up, x_mid_l, y_down);

      drawTriangle(x_mid_r, y_up, x_right, y_mid, x_mid_r, y_down);
      break;

    case LOOP:

      drawCircle(x_mid, y_mid, size / 2 - border_size * 2, 20);

      /*
            STEP_BACK    = 3,
            STEP_FORWARD = 4,
            TIMELINE     = 5,
            LOOP         = 7,
            NONE         = 6
      */
  }
}

void Timeline::drawTriangle(double x1, double y1, double x2, double y2,
                            double x3, double y3) {
  float z = 0.0;

  glBegin(GL_TRIANGLES);
  glVertex3f(x1, y1, z);
  glVertex3f(x2, y2, z);
  glVertex3f(x3, y3, z);
  glEnd();
}

void Timeline::drawRectangle(double x1, double y1, double x2, double y2) {
  float z = 0.0;

  glBegin(GL_QUADS);

  glVertex3f(x1, y1, z);
  glVertex3f(x1, y2, z);
  glVertex3f(x2, y2, z);
  glVertex3f(x2, y1, z);
  glEnd();
}

void Timeline::drawCircle(double cx, double cy, double r, int num_segments) {
  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < num_segments; i++) {
    // Current Angle.
    double theta = 2.0 * 3.1415926 * float(i) / float(num_segments);
    double dx = r * cosf(theta);
    double dy = r * sinf(theta);
    glVertex2f(cx + dx, cy + dy);
  }
  glEnd();
}

void Timeline::drawLine(double x1, double y1, double x2, double y2) {
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
  glEnd();
}

// Returns true iff the given mouse location is within the bounds of this
// timeline.
// This function sets the particular button containing (x, y) to the hover
// state.
bool Timeline::mouse_over(float x, float y) {
  // determine whether (x, y) is in bounds.

  if (mouse_not_in_bounds(x, y)) {
    hover_button_index = NONE;
    return false;
  }

  int buttons_start_x = getButtonStartX();

  if (x < buttons_start_x) {
    hover_button_index = -1;
  } else {
    // FIXME : Set hover_button_index here.
    // Perhaps use getElement for this as well.
    hover_button_index = (x - buttons_start_x) / this->h;
  }

  return true;
}

bool Timeline::mouse_over_timeline(float x, float y) {
  return !mouse_not_in_bounds(x, y) && x < getButtonStartX();
}

bool Timeline::mouse_not_in_bounds(float x, float y) {
  return (this->y > y || y > this->y + this->h || this->x > x ||
          x > this->x + this->w);
}

bool Timeline::mouse_click(float x, float y) {
  // Do not do anything if the mouse is not in bounds.
  if (mouse_not_in_bounds(x, y)) {
    return false;
  }

  e_Element elem = getElement(x, y);

  switch (elem) {
    case NONE:
      selected_button_index = -1;
      return true;

    case REWIND:
      action_rewind();
      selected_button_index = 0;
      return true;

    case STOP:
      action_stop();
      selected_button_index = 1;
      return true;

    // Runs this timeline. Plays from the beginning if at end.
    case PLAY:
      action_play();
      selected_button_index = 2;
      return true;

    case STEP_BACK:
      action_step_backward();
      selected_button_index = 3;
      return true;

    case STEP_FORWARD:
      action_step_forward();
      selected_button_index = 4;
      return true;

    case LOOP:
      action_loop();
      return true;

    case TIMELINE: {
      int button_x = getButtonStartX();
      int line_w = (button_x - this->x);

      double dx = x - this->x;
      double time = dx / line_w;
      current_frame = (int)(time * max_frame);
      isPlaying = false;
      return true;
    }

    default:
      cerr << "e_Element type is not handled." << endl;
      exit(-7);
  }
}

void Timeline::action_rewind() {
  isPlaying = false;
  current_frame = 0;
}

void Timeline::action_goto_end() {
  isPlaying = false;
  current_frame = max_frame;
}

void Timeline::action_stop() { isPlaying = false; }

void Timeline::action_play() {
  isPlaying = true;
  if (current_frame == max_frame) {
    current_frame = 0;
  }
}

void Timeline::action_step_forward(int nFrames) {
  isPlaying = false;
  current_frame = (current_frame + nFrames) % max_frame;
}

void Timeline::action_step_backward(int nFrames) {
  isPlaying = false;
  current_frame = (current_frame + max_frame - nFrames) % max_frame;
}

void Timeline::action_loop() { isLooping = !isLooping; }

Timeline::e_Element Timeline::getElement(int x, int y) {
  if (x < getButtonStartX()) {
    return TIMELINE;
  }

  if (hover_button_index == NONE || hover_button_index < 0) {
    return NONE;
  }

  // Even simpler, return the index of the button that is currently being
  // hovered over.
  // This index cooresponds to the e_Element type for a unique button.
  return (e_Element)hover_button_index;

  // Use integer division to extract the index of the button,
  // which will be the same as the enum value for the button.
  //	return (x - getButtonStartX())/getButtonW();
}

// Returns the current time.
int Timeline::getCurrentFrame() { return current_frame; }

int Timeline::getMaxFrame() { return max_frame; }

bool Timeline::step()  // Update function.
{
  if (!isPlaying) {
    return false;
  }

  if (isLooping) {
    current_frame = (current_frame + 1) % (max_frame + 1);
    return true;
  }

  if (current_frame >= max_frame) {
    current_frame = max_frame;
    isPlaying = false;
    return false;
  }

  current_frame++;
  return true;
}

// Sets the ending time of this timeline.
// REQUIRES : max_frame > 0.
void Timeline::setMaxFrame(int max_frame) {
  if (max_frame <= 0) {
    max_frame = 1;
  }

  this->max_frame = max_frame;

  if (current_frame > max_frame) {
    current_frame = max_frame;
  }
}

// Adds the given time to this timeline's important times set.
// Returns true iff the given time was not already marked.
bool Timeline::markTime(int time) {
  pair<set<int>::iterator, int> ret = times.insert(time);
  return ret.second;
}

// Removes the given time from this timeline's important times set.
// Returns true iff the given time was alread marked.
bool Timeline::unmarkTime(int time) {
  if (times.find(time) != times.end()) {
    times.erase(time);
    return true;
  }

  return false;
}

// Returns true iff the present time is marked.
bool Timeline::isPresentTimeMarked() {
  return times.find(current_frame) != times.end();
}

// Returns the timestep of the next marked time.
// Returns -1 if there are no more remaining important times.
int Timeline::nextImportantTime() {
  // NOTE: Upper bound returns a frame after current_frame.
  set<int>::iterator iter = times.upper_bound(current_frame);

  if (iter == times.end()) {
    return -1;
  }

  return *iter;
}

// Returns the timestep of the last important time that has already been passed.
int Timeline::previousImportantTime() {
  // NOTE : lower bound returns an iter to next > or = element.
  set<int>::iterator iter = times.lower_bound(current_frame);

  if (iter == times.begin()) {
    return -1;
  }

  // Iteratres back one key frame.
  // It we are currently on a key frame,
  // then lower bound would have given us back current_frame.
  iter--;

  return *iter;
}

void Timeline::resize(int w, int h) {
  if (w <= 0 || h <= 0) {
    cerr << "ERROR : Timeline cannot only have positive w and h.\n";
    exit(-7);
  }

  this->w = w;
  this->h = h;
}

void Timeline::move(int x, int y) {
  this->x = x;
  this->y = y;
}

bool Timeline::isCurrentlyPlaying() { return isPlaying; }

void Timeline::action_toggle_playing() { isPlaying = !isPlaying; }

void Timeline::action_goto_next_key_frame() {
  int time_new = nextImportantTime();

  if (time_new >= 0) {
    current_frame = time_new;
  }
}

void Timeline::action_goto_prev_key_frame() {
  int time_new = previousImportantTime();

  if (time_new >= 0) {
    current_frame = time_new;
  }
}

void Timeline::makeLonger(int number) {
  if (number > 0) {
    max_frame += number;
  }
}

void Timeline::makeShorter(int number) {
  if (number > 0 && number < max_frame) {
    max_frame -= number;
  }
}
}
