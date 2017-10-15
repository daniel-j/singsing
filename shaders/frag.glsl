#version 100

#ifdef GL_ES
precision highp float;
#endif

varying vec2 uv;
uniform sampler2D texture;


void main(void) {
    gl_FragColor = vec4(texture2D(texture, uv).rgb, 1.0);
}
