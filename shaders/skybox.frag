#version 330 core

in vec3 vDirection;

uniform samplerCube uSkybox;

out vec4 FragColor;

void main() {
    vec3 color = texture(uSkybox, vDirection).rgb;
    FragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
