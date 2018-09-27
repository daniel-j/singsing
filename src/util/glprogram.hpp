#pragma once

#include <GL/glew.h>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include "./glutils.hpp"

class Program {
 public:
    ~Program() {
        // Cleanup all the things we bound and allocated
        GLint currentProgram;
        GLCall(glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram));
        if (currentProgram == shaderProgram) {
            // disable current program if it's current
            GLCall(glUseProgram(0));
        }
        GLCall(glDeleteProgram(shaderProgram));
    }

    bool load(const std::string &vertexShaderPath, const std::string &fragmentShaderPath) {
        // Generate our shader. This is similar to glGenBuffers() and glGenVertexArray()
        // except that this returns the ID
        shaderProgram = glCreateProgram();

        if (!LoadShader(vertexShaderPath, GL_VERTEX_SHADER)) {
            return false;
        }

        if (!LoadShader(fragmentShaderPath, GL_FRAGMENT_SHADER)) {
            return false;
        }

        // All shaders has been created, now we must put them together into one large object
        auto res = LinkShaders();
        if (!res) return res;

        GLCall(glGetProgramiv(shaderProgram, GL_ACTIVE_UNIFORMS, &uniformCount));
        GLCall(glGetProgramiv(shaderProgram, GL_ACTIVE_ATTRIBUTES, &attributeCount));
        // WebGL: gl.getProgramParameter(program, gl.ACTIVE_UNIFORMS);

        const GLsizei bufSize = 256; // maximum name length
        GLchar name[bufSize]; // variable name in GLSL
        GLsizei length; // name length

        GLint size; // size of the variable
        GLenum type; // type of the variable (float, vec3 or mat4, etc)

        for (GLuint i = 0; i < uniformCount; i++) {
            GLCall(glGetActiveUniform(shaderProgram, i, bufSize, &length, &size, &type, name));
            uniforms[name] = i;
            // printf("Uniform #%d Name: %s\n", i, name);
        }
        for (GLuint i = 0; i < attributeCount; i++) {
            GLCall(glGetActiveAttrib(shaderProgram, i, bufSize, &length, &size, &type, name));
            attributes[name] = i;
            // printf("Attribute #%d Name: %s\n", i, name);
        }
        return true;
    }

    bool LoadShader(const std::string &fileName, GLenum shaderType) {
        // std::cout << "Loading Shader: " << fileName << std::endl;

        int shaderId = CreateShader(fileName, shaderType);

        if (TryCompileShader(shaderId, fileName)) {
            GLCall(glAttachShader(shaderProgram, shaderId));
            shaderIds.push_back(shaderId);

            return true;
        }

        return false;
    }

    void use() {
        // Load the shader into the rendering pipeline
        GLCall(glUseProgram(shaderProgram));
    }

    GLint attribute(const std::string& name) {
        auto it = attributes.find(name);
        if (it != attributes.end()) {
            return it->second;
        }
        return GLCall(glGetAttribLocation(shaderProgram, name.c_str()));
    }
    GLint uniform(const std::string& name) {
        auto it = uniforms.find(name);
        if (it != uniforms.end()) {
            return it->second;
        }
        return GLCall(glGetUniformLocation(shaderProgram, name.c_str()));
    }

    void setUniform(const std::string& name, GLfloat v0) {
        GLCall(glUniform1f(uniform(name), v0));
    }
    void setUniform(const std::string& name, GLfloat v0, GLfloat v1)  {
        GLCall(glUniform2f(uniform(name), v0, v1));
    }
    void setUniform(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2)  {
        GLCall(glUniform3f(uniform(name), v0, v1, v2));
    }
    void setUniform(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)  {
        GLCall(glUniform4f(uniform(name), v0, v1, v2, v3));
    }

    void setUniformi(const std::string& name, GLint v0) {
        GLCall(glUniform1i(uniform(name), v0));
    }
    void setUniformi(const std::string& name, GLint v0, GLint v1) {
        GLCall(glUniform2i(uniform(name), v0, v1));
    }

    void setUniformui(const std::string& name, GLuint v0) {
        GLCall(glUniform1ui(uniform(name), v0));
    }
    void setUniformui(const std::string& name, GLuint v0, GLuint v1) {
        GLCall(glUniform2ui(uniform(name), v0, v1));
    }

 private:
    // Tries to compile the shader. Returns false if something fails
    bool TryCompileShader(int shaderId, const std::string &fileName) {
        // Compile the vertex shader
        GLCall(glCompileShader(shaderId));

        // Ask OpenGL if the shaders was compiled
        int wasCompiled = 0;
        GLCall(glGetShaderiv(shaderId, GL_COMPILE_STATUS, &wasCompiled));

        PrintShaderCompilationErrorInfo(shaderId, fileName);

        // Return false if compilation failed
        return (wasCompiled != 0);
    }

    // Create shader and set the source
    int CreateShader(const std::string &fileName, GLenum shaderType) {
        // Read file as std::string
        std::string str = ReadFile(fileName.c_str());

        // c_str() gives us a const char*, but we need a non-const one
        char* src = const_cast<char*>(str.c_str());
        int32_t size = str.length();

        // Create an empty vertex shader handle
        int shaderId = GLCall(glCreateShader(shaderType));

        // Send the vertex shader source code to OpenGL
        GLCall(glShaderSource(shaderId , 1, &src, &size));

        return shaderId;
    }

    bool LinkShaders() {
        // At this point, our shaders will be inspected/optized and the binary code generated
        // The binary code will then be uploaded to the GPU
        GLCall(glLinkProgram(shaderProgram));

        // Verify that the linking succeeded
        int isLinked;
        GLCall(glGetProgramiv(shaderProgram, GL_LINK_STATUS, (int *)&isLinked));

        if (isLinked == false)
            PrintShaderLinkingError(shaderProgram);

        for ( auto i : shaderIds)
            GLCall(glDetachShader(shaderProgram, i));
        for ( auto i : shaderIds)
            GLCall(glDeleteShader(i));

        return isLinked != 0;
    }

    std::string ReadFile(const char* file) {
        // Open file
        std::ifstream t(file);

        // Read file into buffer
        std::stringstream buffer;
        buffer << t.rdbuf();

        // Make a std::string and fill it with the contents of buffer
        std::string fileContent = buffer.str();

        return fileContent;
    }

    void PrintShaderLinkingError(int32_t shaderId) {
        std::cerr << "=======================================\n";
        std::cerr << "Shader linking failed : " << std::endl;

        // Find length of shader info log
        int maxLength;
        GLCall(glGetProgramiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength));

        std::cerr << "Info Length : " << maxLength << std::endl;

        // Get shader info log
        char* shaderProgramInfoLog = new char[maxLength];
        GLCall(glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, shaderProgramInfoLog));

        std::cerr << "Linker error message : " << shaderProgramInfoLog << std::endl;

        // Handle the error by printing it to the console
        delete[] shaderProgramInfoLog;
        return;
    }

    // If something went wrong whil compiling the shaders, we'll use this function to find the error
    void PrintShaderCompilationErrorInfo(int32_t shaderId, const std::string &fileName) {
        // Find length of shader info log
        int maxLength;
        GLCall(glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength));

        // Get shader info log
        char* shaderInfoLog = new char[maxLength];
        GLCall(glGetShaderInfoLog(shaderId, maxLength, &maxLength, shaderInfoLog));

        std::string log = shaderInfoLog;

        if (log.length() && maxLength) {
            std::cerr << "=======[ Shader compilation failed ]=======" <<std::endl <<
                         fileName << std::endl <<
                         shaderInfoLog <<
                         "===========================================" << std::endl;
        }

        // Print shader info log
        delete[] shaderInfoLog;
    }

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
