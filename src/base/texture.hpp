#pragma once

#include <glad/glad.h>

class Texture {
    GLuint texNum;
    float x = 0.0;
    float y = 0.0;
    float z = 0.0;
    float w = 0.0;
    float h = 0.0;
    float scaleW = 1.0;
    float scaleH = 1.0;
    float rot = 0.0;
    float rightScale = 1.0;
    float leftScale = 1.0;
    float intensity = 1.0;
    float colR = 0.0;
    float colG = 0.0;
    float colB = 0.0;
    float texW = 1.0;
    float texH = 1.0;
    float texX1;
    float texY1;
    float texX2;
    float texY2;
public:
    Texture();
};

class TextureUnit {
private:
    //TextureDatabase database;
};
