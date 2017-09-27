#ifndef CMU462_VIEWER_H
#define CMU462_VIEWER_H

#include "renderer.h"
#include "osdtext.h"

#include <chrono>

#include "GLFW/glfw3.h"

namespace CMU462 {

/**
 * Provides OpenGL context, window display, and event handling routines.
 * A user application may draw to the window's context by providing
 * a user renderer. The viewer manages other display components such as the
 * zoom views, text OSD, etc. It also takes care of window event handling and
 * event passing, through which the renderer may interact with user inputs.
 */
class Viewer {
 public:

  /**
   * Constructor.
   * Creates a new viewer with the default title (CMU462).
   */
  Viewer( void );

  /**
   * Constructor.
   * Creates a new viweer with the given title.
   */
  Viewer( const char* title );

  /**
   * Destructor.
   * Destroys the viewer instance and frees memory.
   * Note that this does not change the user space renderer.
   */
  ~Viewer( void );

  /**
   * Initialize the viewer.
   * This will open up a window and install all the event handlers
   * and make the viewer ready for drawing.
   */
  void init( void );

  /**
   * Start the drawing loop of the viewer.
   * Once called this will block until the viewer is close.
   */
  void start( void );
  
  /**
   * Set a user space renderer.
   * The viewer will use the given user space renderer in drawing.
   * \param renderer The user space renderer to use in the viewer.
   */
  void set_renderer( Renderer *renderer );
  
  /**
   * Show an error to the user in the GUI.
   * Continues drawing internally to process click, but does not return until error is dismissed.
   * \param 
   */
  static void showError(std::string errorText, bool isFatal=false);

 private:
  
  /**
   * Main update loop.
   */
  static void update( void );

  
  /**
   * Draw information view.
   */
  static void drawInfo( void );

  // window event callbacks
  static void err_callback( int error, const char* description );
  static void key_callback( GLFWwindow* window, int key, int scancode, int action, int mods );
  static void char_callback( GLFWwindow* window, unsigned int codepoint );
  static void resize_callback( GLFWwindow* window, int width, int height );
  static void cursor_callback( GLFWwindow* window, double xpos, double ypos );
  static void scroll_callback( GLFWwindow* window, double xoffset, double yoffset);
  static void mouse_button_callback( GLFWwindow* window, int button, int action, int mods );

  // HDPI display
  static bool HDPI;

  // framerate related timeers
  static int framecount;
  static std::chrono::time_point<std::chrono::system_clock> sys_last;
  static std::chrono::time_point<std::chrono::system_clock> sys_curr;

  // info toggle
  static bool showInfo;

  // window properties
  static GLFWwindow* window;
  static size_t buffer_w;
  static size_t buffer_h;

  // user space renderer
  static Renderer* renderer;

  // on-screen display
  static OSDText* osd_text;
  static int line_id_renderer;
  static int line_id_framerate;

  // Error Dialogs //
  static void drawError();
  static bool showingError;
  static bool errorDismissed;
  static bool errorFatal;
  static std::string errorMessage;


}; // class Viewer

} // namespace CMU462

#endif // CMU462_VIEWER_H
