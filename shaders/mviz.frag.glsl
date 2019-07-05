#version 330
precision highp float;

uniform vec2 iRes;
uniform float iTime;
uniform int iFrame;
uniform float iNumGeomIters;
uniform sampler1D iSound;
uniform sampler1D iFreq;

out vec4 c;
void main() {
    c = vec4(0.1, 0.0, 0.1, 1.0);
}
