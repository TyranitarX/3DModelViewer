#version 460 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
struct Light
{
//0直射光 1点光源 2聚焦光源
    int type;
    vec3 position;
    vec3 direction;
//聚光范围角的cos值
    float cutOff;
//外围弱化光圈
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

//点光源 弱化参数
    float constant;
    float linear;
    float quadratic;
};

uniform int lightNumbers;
uniform Light[5] light;

//直射光
vec3 calcDirLight(Light light, vec3 norm, vec3 viewDir){
    vec3 lightDir = normalize(-light.direction);
    //直射光环境光照
    vec3 ambient = light.ambient  * vec3(texture(texture_diffuse1, TexCoords));
    //漫反射光照
    vec3 diffuse = (max(dot(lightDir, norm), 0.0) * vec3(texture(texture_diffuse1, TexCoords))) * light.diffuse;
    //    //高光
    //    vec3 reflectDir = reflect(light.direction, norm);
    //    float spec = pow(max(dot(reflectDir,viewDir), 0.0), 32.0f);
    //    vec3 specular = spec * light.specular * vec3(texture(texture_specular1, TexCoords));
    return ambient + diffuse;
}

//点光源
vec3 calcPointLight(Light light, vec3 norm, vec3 viewDir, vec3 lightDir)
{
    //环境光照
    vec3 ambient = light.ambient  * vec3(texture(texture_diffuse1, TexCoords));
    //漫反射光照
    vec3 diffuse = (max(dot(lightDir, norm), 0.0) * vec3(texture(texture_diffuse1, TexCoords))) * light.diffuse;
    //    //镜面反射光照
    //    vec3 reflectDir = reflect(-lightDir, norm);
    //    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
    //    vec3 specular = vec3(texture(texture_specular1, TexCoords)) * spec * light.specular;

    //光线弱化
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
    light.quadratic * (distance * distance));
    diffuse *= attenuation;
    //    specular *= attenuation;
    return diffuse + ambient;
}

//聚光
vec3 calcSpotLight(Light light, vec3 norm, vec3 viewDir, vec3 lightDir)
{
    //环境光照
    vec3 ambient = light.ambient  * vec3(texture(texture_diffuse1, TexCoords));
    //漫反射光照
    vec3 diffuse = (max(dot(lightDir, norm), 0.0) * vec3(texture(texture_diffuse1, TexCoords))) * light.diffuse;
    //    //镜面反射
    //    vec3 reflectDir = reflect(-lightDir, norm);
    //    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
    //    vec3 specular = vec3(texture(texture_specular1, TexCoords)) * spec * light.specular;
    //
    //    float theta = dot(-normalize(light.direction), -normalize(lightDir));
    //    float intensity = clamp((theta - light.outerCutOff) / (light.cutOff - light.outerCutOff) , 0.0, 1.0);
    //    diffuse *=  intensity;
    //    specular *= intensity;

    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
    light.quadratic * (distance * distance));
    diffuse *= attenuation;
    //    specular *= attenuation;
    return diffuse + ambient;
}

void main()
{
    vec3 result = vec3(0.0f);
    for (int i=0; i < lightNumbers; i++)
    {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(light[i].position - FragPos);
        vec3 viewDir = normalize(-FragPos);
        switch (light[i].type){
            case 0:
            result += calcDirLight(light[i], norm, viewDir);
            break;
            case 1:
            result += calcPointLight(light[i], norm, viewDir, lightDir);
            break;
            case 2:
            result += calcSpotLight(light[i], norm, viewDir, lightDir);
            break;
            default :
            break;
        }
    }
    FragColor = vec4(result, 1.0f);
}