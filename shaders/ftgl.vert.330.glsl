#version 330

in vec2 vertex;
in vec2 texcoord;
in vec4 color;

uniform vec2 viewportSize;
uniform vec2 viewOffset;

out vec2 uv;
out vec4 textcolor;

void main() {
    uv = texcoord;
    textcolor = color;
    gl_Position = vec4(((vertex + viewOffset) * 2.0 / viewportSize - 1.0) * vec2(1, -1), 0.0, 1.0);
}
