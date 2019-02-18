#pragma once

#include <iostream>

#include "GL/glew.h"
#include "CMU462/CMU462.h"

static void check_gl_error(size_t lineno, const char *filename) {
  GLenum err;
  const GLubyte *str;
  while((err = glGetError()) != GL_NO_ERROR)
  {
    str = gluErrorString(err);
    std::cout << "GL ERROR " << filename << ":" << lineno << std::endl;
    std::cout << "    " << str << std::endl;
  }
}
#define GL_ERROR() check_gl_error(__LINE__, __FILE__)

static std::string check_fbo(GLuint fbo) {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  GLuint ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  switch (ret) {
    case GL_FRAMEBUFFER_COMPLETE: return "complete";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "incomplete attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "incomplete missing attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "incomplete draw buffer";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "incomplete read buffer";
    case GL_FRAMEBUFFER_UNSUPPORTED: return "unsupported";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "incomplete multisample";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "incomplete layer targets";
  }
  return "error";
}