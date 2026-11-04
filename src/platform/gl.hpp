#pragma once
// Chon header GLUT dung cho tung he dieu hanh.
#if defined(__APPLE__)
#  ifndef GL_SILENCE_DEPRECATION
#    define GL_SILENCE_DEPRECATION
#  endif
#  include <GLUT/glut.h>
#elif defined(_WIN32)
#  include <GL/freeglut.h>
#else
#  include <GL/glut.h>
#endif
