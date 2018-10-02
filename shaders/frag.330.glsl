#version 330

uniform vec4 bgColor;
uniform sampler2D u_texture;
uniform float textureOpacity;

in vec2 uv;

out vec4 fragColor;

void main(void) {
    vec4 tex = texture(u_texture, uv);
    tex.a *= textureOpacity;
    fragColor = mix(bgColor, tex, tex.a);
}
