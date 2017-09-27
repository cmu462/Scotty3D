#include "viewer.h"

#include <stdio.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "GL/glew.h"

#include "console.h"

using namespace std;
using namespace chrono;

#define DEFAULT_W 960
#define DEFAULT_H 640

namespace CMU462 {

// HDPI display
bool Viewer::HDPI;

// framecount & related timeers
int Viewer::framecount;
time_point<system_clock> Viewer::sys_last; 
time_point<system_clock> Viewer::sys_curr; 

// draw toggles
bool Viewer::showInfo = true;

// window properties
GLFWwindow* Viewer::window;
size_t Viewer::buffer_w;
size_t Viewer::buffer_h;

// user space renderer
Renderer* Viewer::renderer; 

// on-screen display
OSDText* Viewer::osd_text;
int Viewer::line_id_renderer;
int Viewer::line_id_framerate;

// Error dialogs
bool Viewer::showingError = false;
bool Viewer::errorDismissed = false;
bool Viewer::errorFatal = false;
std::string Viewer::errorMessage = "";

Viewer::Viewer() {

}

Viewer::~Viewer() {

  glfwDestroyWindow(window);
  glfwTerminate();
  
  // free resources
  delete renderer;
  delete osd_text;
}


void Viewer::init() {

  // initialize glfw
  glfwSetErrorCallback( err_callback );
  if( !glfwInit() ) {
    out_err("Error: could not initialize GLFW!");
    exit( 1 );
  }

  // create window
  string title = renderer ? "CMU462: " + renderer->name() : "CMU462";
  window = glfwCreateWindow( DEFAULT_W, DEFAULT_H, title.c_str(), NULL, NULL );
  if (!window) {
    out_err("Error: could not create window!");
    glfwTerminate();
    exit( 1 );
  }

  // set context
  glfwMakeContextCurrent( window );
  glfwSwapInterval(1);

  // framebuffer event callbacks
  glfwSetFramebufferSizeCallback( window, resize_callback );
  
  // key event callbacks
  glfwSetKeyCallback( window, key_callback );

  // character event callbacks
  glfwSetCharCallback( window, char_callback );
  
  // cursor event callbacks
  glfwSetCursorPosCallback( window, cursor_callback );

  // wheel event callbacks
  glfwSetScrollCallback(window, scroll_callback);  
  
  // mouse button callbacks
  glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);
  glfwSetMouseButtonCallback(window, mouse_button_callback);

  // initialize glew
  if (glewInit() != GLEW_OK) {
    out_err("Error: could not initialize GLEW!");
    glfwTerminate();
    exit( 1 );
  }

  // enable alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // resize components to current window size, get DPI
  glfwGetFramebufferSize(window, (int*) &buffer_w, (int*) &buffer_h );
  if( buffer_w > DEFAULT_W ) HDPI = true;

  // initialize renderer if already set
  if (renderer){
    if (HDPI) renderer->use_hdpi_reneder_target();
    renderer->init();
  } 

  // initialize status OSD
  osd_text = new OSDText();
  if (osd_text->init(HDPI) < 0) {
    out_err("Error: could not initialize on-screen display!");
    exit( 1 );
  }
  
  // add lines for renderer and fps
  line_id_renderer  = osd_text->add_line(-0.95,  0.90, "Renderer", 
                                          18, Color(0.15, 0.5, 0.15));
  line_id_framerate = osd_text->add_line(-0.98, -0.96, "Framerate", 
                                          14, Color(0.15, 0.5, 0.15));

  // resize elements to current size
  resize_callback(window, buffer_w, buffer_h);

}

void Viewer::start() {

  // start timer
  sys_last = system_clock::now();

  // run update loop
  while( !glfwWindowShouldClose( window ) ) {  
    update();
  }
}

void Viewer::set_renderer(Renderer *renderer) {
  this->renderer = renderer;
}

void Viewer::update() {
  
  // clear frame
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // run user renderer
  if (renderer) {
    renderer->render();
  }

  // draw info
  if( showInfo ) {
    drawInfo();        
  } 
  drawError();

  // swap buffers
  glfwSwapBuffers(window); 

  // poll events
  glfwPollEvents();
}

// Displays an error to the user, does not return until the user clicks
// to dismiss the error.
void Viewer::showError(std::string errorText, bool isFatal)
{

    // Set the error
    showingError = true;
    errorDismissed = false;
    errorMessage = errorText;
    errorFatal = isFatal;

    // Continue poll/draw loop so we redraw the screen and continue user input
    while(!errorDismissed) {
        Viewer::update();
    }

    // Panic quit if the error was fatal
    if(isFatal) {
        cerr << "FATAL ERROR: " << errorText << endl;
        exit(EXIT_FAILURE);
    }

    showingError = false;
}

void Viewer::drawInfo() {

  // compute timers - fps is update every second
  sys_curr = system_clock::now();
  double elapsed = ((duration<double>) (sys_curr - sys_last)).count();
  if (elapsed >= 1.0f) {

    // update framecount OSD
    Color c = framecount < 20 ? Color(1.0, 0.35, 0.35) : Color(0.15, 0.5, 0.15);
    osd_text->set_color(line_id_framerate, c);
    string framerate_info = "Framerate: " + to_string(framecount) + " fps";
    osd_text->set_text(line_id_framerate, framerate_info);

    // reset timer and counter
    framecount = 0;
    sys_last = sys_curr; 

  } else {

    // increment framecount
    framecount++;
  
  }

  // udpate renderer OSD
  // TODO: This is done on every update and it shouldn't be!
  // The viewer should only update when the renderer needs to
  // update the info text. 
  if (renderer) {
    string renderer_info = renderer->info();
    osd_text->set_text(line_id_renderer, renderer_info);
  } else {
    string renderer_info = "No input renderer";
    osd_text->set_text(line_id_renderer, renderer_info);
  }

  // render OSD
  osd_text->render();

}

void Viewer::drawError() {

    if(!showingError) {
        return;
    }
   
    // GL prep
    // Note: This was copied in, I think it's a bit overkill
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, buffer_w, buffer_h);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, buffer_w, buffer_h, 0, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0, 0, -1);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    
    // Style based on error type
    std::string errorTitle;
    std::string clickPrompt;
    Color errorColor;
    if(errorFatal) {
        errorColor = Color(.85, .3, .3);
        errorTitle = "FATAL ERROR";
        clickPrompt = "Click anywhere to exit.";
    } else { 
        errorColor = Color(1.0, .65, .2);
        errorTitle = "ERROR";
        clickPrompt = "Click anywhere to dismiss.";
    }

    // == Draw the error overlay
    int textSize = 16;
    int textHeight = HDPI ? 2*textSize : textSize;
    glBegin(GL_QUADS);

    // Gray out the background
    glColor4f(.5, .5, .5, .8);
    glVertex2f(0,0);
    glVertex2f(0,buffer_h);
    glVertex2f(buffer_w, buffer_h);
    glVertex2f(buffer_w, 0);

    // Draw a scary red box
    size_t boxHeight = 11*textHeight;
    boxHeight = std::min(boxHeight, 2*buffer_h/3);
    size_t boxWidth = 2*buffer_w/3;
    size_t boxX0 = buffer_w/6;
    size_t boxY0 = buffer_h/6;
    glColor4f(errorColor.r, errorColor.g, errorColor.b, 0.8);
    glVertex2f(boxX0 + boxWidth, boxY0);
    glVertex2f(boxX0 + boxWidth, boxY0 + boxHeight);
    glVertex2f(boxX0, boxY0 + boxHeight);
    glVertex2f(boxX0, boxY0);

    glEnd();
    
    // == Draw the text
    // Note that the coordinates in each command are transformed to the [-1,1] coordinates used in OSDText.

    int id1 = osd_text->add_line(2.0*(boxX0 + 2*textHeight)/buffer_w - 1.0,
                                -2.0*(boxY0 + 3*textHeight)/buffer_h + 1.0, 
                                 errorTitle, 2*textSize, Color(1,1,1));
    int id2 = osd_text->add_line(2.0*(boxX0 + 2*textHeight)/buffer_w - 1.0,
                                -2.0*(boxY0 + 6*textHeight)/buffer_h + 1.0, 
                                 errorMessage, 2*textSize, Color(1,1,1));
    int id3 = osd_text->add_line(2.0*(boxX0 + 2*textHeight)/buffer_w - 1.0,
                                -2.0*(boxY0 + 9*textHeight)/buffer_h + 1.0, 
                                 clickPrompt, textSize, Color(1,1,1));
    
    osd_text->render();
    osd_text->del_line(id1);
    osd_text->del_line(id2);
    osd_text->del_line(id3);

    // GL cleanup
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}


void Viewer::err_callback( int error, const char* description ) {
    out_err( "GLFW Error: " << description );
}

void Viewer::resize_callback( GLFWwindow* window, int width, int height ) {

  // get framebuffer size
  int w, h; 
  glfwGetFramebufferSize(window, &w, &h );
    
  // update buffer size
  buffer_w = w; buffer_h = h;
  glViewport( 0, 0, buffer_w, buffer_h );

  // resize on-screen display
  osd_text->resize(buffer_w, buffer_h);

  // resize render if there is a user space renderer
  if (renderer) renderer->resize( buffer_w, buffer_h );  
}

void Viewer::cursor_callback( GLFWwindow* window, double xpos, double ypos ) {

  // forward pan event to renderer
  if( HDPI ) {
    float cursor_x = 2 * xpos;
    float cursor_y = 2 * ypos;
    renderer->cursor_event(cursor_x, cursor_y);
  } else {
    float cursor_x = xpos;
    float cursor_y = ypos;
    renderer->cursor_event(cursor_x, cursor_y);
  }

}

void Viewer::scroll_callback( GLFWwindow* window, double xoffset, double yoffset) {
    
  // Srolls do nothing while showing an error
  if(showingError) {
      return;
  }

  renderer->scroll_event(xoffset, yoffset);

}


void Viewer::mouse_button_callback( GLFWwindow* window, int button, int action, int mods ) {
        
  // While showing an error, the only thing the mouse does is dismiss the error
  if(showingError) {
    if(action == EVENT_PRESS) {
      errorDismissed = true;
    }
    return;
  }

  renderer->mouse_event( button, action, mods );

}

void Viewer::key_callback( GLFWwindow* window, 
                           int key, int scancode, int action, int mods ) {

  if (action == GLFW_PRESS) {
    if( key == GLFW_KEY_ESCAPE ) { 
      glfwSetWindowShouldClose( window, true ); 
    } else if( key == GLFW_KEY_GRAVE_ACCENT ){
      showInfo = !showInfo;
    } 
  }
    
  // Keys do nothing while showing an error
  if(showingError) {
      return;
  }
  
  renderer->keyboard_event( key, action, mods );
}

void Viewer::char_callback( GLFWwindow* window, 
                            unsigned int codepoint ) {
                              
  // Keys do nothing while showing an error
  if(showingError) {
      return;
  }

  renderer->char_event( codepoint );
}



} // namespace CMU462

