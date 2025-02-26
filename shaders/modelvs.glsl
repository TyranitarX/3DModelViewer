#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 viewMatrix;
uniform mat4 transMatrix;
uniform mat3 normalMatrix;

void main()
{
    gl_Position = transMatrix *  vec4(aPos, 1.0);
    Normal = normalMatrix * aNormal;
    FragPos = vec3(viewMatrix * vec4(aPos, 1.0f));
    TexCoords = aTexCoords;
}