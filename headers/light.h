//
// Created by tyran on 2024/10/16.
//

#ifndef LIGHT_H
#define LIGHT_H
#include <camera.h>
#include <shader.h>

//不要直接使用class 此处仅作为interface
class Light {
public:
    unsigned int id, VAO, VBO;
    glm::vec3 direction{};
    glm::vec3 ambient{};
    glm::vec3 diffuse{};
    glm::vec3 specular{};
    glm::vec3 position{};

    Light(unsigned int id, glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse,
          glm::vec3 specular) {
        this->id = id;
        this->direction = direction;
        this->ambient = ambient;
        this->diffuse = diffuse;
        this->specular = specular;
    }

    void addToShader(const Shader *shader) {
        shader->setVec3("light[" + std::to_string(id) + "].direction", direction);
        shader->setVec3("light[" + std::to_string(id) + "].ambient", ambient);
        shader->setVec3("light[" + std::to_string(id) + "].diffuse", diffuse);
        shader->setVec3("light[" + std::to_string(id) + "].specular", specular);
    }
};

class DirectionalLight : public Light {
public:
    int type;

    DirectionalLight(unsigned int id, glm::vec3 direction,
                     glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular) : Light(
        id, direction, ambient, diffuse, specular) {
        this->type = 0;
    }

    void addToShader(const Shader *shader) {
        shader->setVec3("light[" + std::to_string(this->id) + "].direction", this->direction);
        shader->setVec3("light[" + std::to_string(this->id) + "].ambient", this->ambient);
        shader->setVec3("light[" + std::to_string(this->id) + "].diffuse", this->diffuse);
        shader->setVec3("light[" + std::to_string(this->id) + "].specular", this->specular);
        shader->setInt("light[" + std::to_string(this->id) + "].type", this->type);
    }
};

class PointLight : public Light {
public:
    int type;
    glm::vec3 position{};
    float constant;
    float linear;
    float quadratic;

    PointLight(unsigned int id, glm::vec3 position, glm::vec3 direction,
               glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear,
               float quadratic) : Light(id, direction, ambient, diffuse, specular) {
        this->type = 1;
        this->position = position;
        this->constant = constant;
        this->linear = linear;
        this->quadratic = quadratic;
        setUpLight();
    }

    void addToShader(const Shader *shader) {
        shader->setVec3("light[" + std::to_string(this->id) + "].direction", this->direction);
        shader->setVec3("light[" + std::to_string(this->id) + "].ambient", this->ambient);
        shader->setVec3("light[" + std::to_string(this->id) + "].diffuse", this->diffuse);
        shader->setVec3("light[" + std::to_string(this->id) + "].specular", this->specular);
        shader->setInt("light[" + std::to_string(this->id) + "].type", this->type);
        shader->setVec3("light[" + std::to_string(id) + "].position", position);
        shader->setFloat("light[" + std::to_string(id) + "].constant", constant);
        shader->setFloat("light[" + std::to_string(id) + "].linear", linear);
        shader->setFloat("light[" + std::to_string(id) + "].quadric", quadratic);
    }

    void Draw(const Shader *shader, Camera *camera, glm::mat4 projection) const {
        shader->use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::scale(model, glm::vec3(0.2f));
        shader->setMat4("model", model);
        shader->setMat4("view", camera->GetViewMatrix());
        shader->setMat4("projection", projection);
        glBindVertexArray(this->VAO);
        glDrawArrays(GL_LINES, 0, 6);
    }

private:
    void setUpLight() {
        float vertices[] = {
            position.x, position.y, position.z, 1.0f, 0.0f, 0.0f,
            position.x + 1.0f, position.y, position.z, 1.0f, 0.0f, 0.0f,
            position.x, position.y, position.z, 0.0f, 1.0f, 0.0f,
            position.x, position.y + 1.0f, position.z, 0.0f, 1.0f, 0.0f,
            position.x, position.y, position.z, 0.0f, 0.0f, 1.0f,
            position.x, position.y, position.z + 1.0f, 0.0f, 0.0f, 1.0f,
        };
        glGenBuffers(1, &this->VBO);
        glGenVertexArrays(1, &this->VAO);
        glBindVertexArray(this->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
};

class SpotLight : public PointLight {
public:
    float cutOff;
    float outerCutOff;

    SpotLight(unsigned int id, glm::vec3 position, glm::vec3 direction,
              glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant, float linear,
              float quadratic, float cutOff, float outerCutOff) : PointLight(
        id, position, direction, ambient, diffuse, specular, constant, linear, quadratic) {
        this->type = 2;
        this->cutOff = cutOff;
        this->outerCutOff = outerCutOff;
    }

    void addToShader(const Shader *shader) {
        shader->setInt("light[" + std::to_string(id) + "].type", 2);
        shader->setFloat("light[" + std::to_string(id) + "].cutOff", cutOff);
        shader->setFloat("light[" + std::to_string(id) + "].outerCutOff", outerCutOff);
    }
};
#endif //LIGHT_H
