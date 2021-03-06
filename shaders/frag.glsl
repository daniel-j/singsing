#version 110

#ifdef GL_ES
precision highp float;
#endif

uniform vec4 bgColor;
varying vec2 uv;
uniform sampler2D u_texture;
uniform float textureOpacity;

void main(void) {
    vec4 tex = texture2D(u_texture, uv);
    tex.a *= textureOpacity;
    gl_FragColor = mix(bgColor, tex, tex.a);
}
