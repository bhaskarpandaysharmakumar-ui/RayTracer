#version 430 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;

void main() {
    float gamma = 2.2;
    vec3 texColor = texture(tex, TexCoords).rgb;
    vec3 gammaCorrectColor = pow(texColor, vec3(1.0/gamma));
    FragColor = vec4(gammaCorrectColor, 1.0);
}