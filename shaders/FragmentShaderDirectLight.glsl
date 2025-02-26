#version 460 core
in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;
out vec4 FragColor;

//材质影响光照的分量
struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct Light
{
    vec3 direction;
    vec3 color;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform Light light;

void main()
{
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(-light.direction);
    float ambientStrength = 0.01;
    //环境光照
    vec3 ambient = light.ambient * ambientStrength * vec3(texture(material.diffuse, texCoords));
    //漫反射光照
    vec3 diffuse = (max(dot(lightDir, norm), 0.0) * vec3(texture(material.diffuse, texCoords))) * light.diffuse;
    vec3 viewDir = normalize(-fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = vec3(texture(material.specular, texCoords)) * spec * light.specular;
    vec3 result = (diffuse + ambient + specular);
    FragColor = vec4(result, 1.0f);
}