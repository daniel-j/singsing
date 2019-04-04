#pragma once

#include <glad/glad.h>

void CheckOpenGLError(const char* stmt, const char* fname, int line);

#define _DEBUG
#ifdef _DEBUG
    #define GLCall(stmt) stmt; CheckOpenGLError(#stmt, __FILE__, __LINE__)
#else
    #define GLCall(stmt) stmt
#endif
