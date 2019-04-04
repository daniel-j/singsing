#pragma once

#include <glad/glad.h>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include "./glutils.hpp"

class Program {
 public:
    ~Program();

    bool load(const std::string &vertexShaderPath, const std::string &fragmentShaderPath/*, const std::string &geometryShaderPath = ""*/);

    bool LoadShader(const std::string &fileName, GLenum shaderType);

    void use();

    GLint attribute(const std::string& name);
    GLint uniform(const std::string& name);

    void setUniform(const std::string& name, GLfloat v0);
    void setUniform(const std::string& name, GLfloat v0, GLfloat v1);
    void setUniform(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2);
    void setUniform(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

    void setUniformi(const std::string& name, GLint v0);
    void setUniformi(const std::string& name, GLint v0, GLint v1);

    void setUniformui(const std::string& name, GLuint v0);
    void setUniformui(const std::string& name, GLuint v0, GLuint v1);

 private:
    // Tries to compile the shader. Returns false if something fails
    bool TryCompileShader(int shaderId, const std::string &fileName);

    // Create shader and set the source
    int CreateShader(const std::string &fileName, GLenum shaderType);

    bool LinkShaders();

    std::string ReadFile(const char* file);

    void PrintShaderLinkingError(int32_t shaderId);

    // If something went wrong whil compiling the shaders, we'll use this function to find the error
    void PrintShaderCompilationErrorInfo(int32_t shaderId, const std::string &fileName);

    // The handle to our shader program
    GLuint shaderProgram;

    std::vector<int32_t> shaderIds;

    // The handles to the induvidual shader
    GLuint vertexshader, fragmentShader;

    int attributeCount = -1;
    int uniformCount = -1;
    std::map<std::string, GLint> attributes;
    std::map<std::string, GLint> uniforms;
};
