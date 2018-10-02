#version 330

in vec2 position;
in vec2 texcoord;

uniform vec2 viewportSize;
uniform vec2 viewSize;
uniform vec2 viewOffset;

out vec2 uv;

void main(void) {
    uv = texcoord;
    gl_Position = vec4(((position * viewSize + viewOffset) * 2.0 / viewportSize - 1.0) * vec2(1, -1), 0.0, 1.0);
}
