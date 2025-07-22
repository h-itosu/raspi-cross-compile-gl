#pragma once
#include <GLES2/gl2.h>

GLuint createShader(GLenum type, const char *source);
GLuint createProgram(const char *vertexSource, const char *fragmentSource);
GLuint createProgramFromFiles(const char *vertexPath, const char *fragmentPath);
