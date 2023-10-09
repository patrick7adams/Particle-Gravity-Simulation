#version 330 core
out vec4 FragColor;

uniform ivec2 resolution;
uniform float time;

in vec3 new_colors;

void main() {
    FragColor = vec4(new_colors, 1.0);
}