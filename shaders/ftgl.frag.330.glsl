#version 330

uniform sampler2D u_texture;

in vec2 uv;
in vec4 textcolor;

out vec4 fragColor;

void main() {
    float a = texture(u_texture, uv).r;
    fragColor = vec4(textcolor.rgb, textcolor.a * a);
}
