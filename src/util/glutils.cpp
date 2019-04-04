#include "glutils.hpp"
#include <stdio.h>
#include <stdlib.h>


void CheckOpenGLError(const char* stmt, const char* fname, int line) {
    while (GLenum err = glGetError() != GL_NO_ERROR) {
        printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        abort();
    }
}
