#pragma once

void CheckOpenGLError(const char* stmt, const char* fname, int line) {
    while (GLenum err = glGetError() != GL_NO_ERROR) {
        printf("OpenGL error %08x, at %s:%i - for %s\n", err, fname, line, stmt);
        abort();
    }
}
#define _DEBUG
#ifdef _DEBUG
    #define GLCall(stmt) stmt; CheckOpenGLError(#stmt, __FILE__, __LINE__)
#else
    #define GLCall(stmt) stmt
#endif
