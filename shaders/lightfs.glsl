#version 460 core
in vec3 oaColor;
out vec4 lightColor;

void main(){
    lightColor = vec4(oaColor, 1.0f);
}