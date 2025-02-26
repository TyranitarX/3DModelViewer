#version 460 core
layout (location = 0) in vec3 aPos;

out vec3 normal;

uniform mat4 transform;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;


void main()
{
    gl_Position = transform * projection * view * model * vec4(aPos, 1.0);
}