#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "lpng/png.h"


#include "stb_image.h"
#include <string>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

unsigned int LoadEmbeddedTexture(const aiTexture *texture);

ModelMaterial loadOriMaterials(aiMaterial *mat);

class Model {
public:
    // model data
    vector<ModelTexture> textures_loaded;
    // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh> meshes;
    string directory;
    bool gammaCorrection;
    // 模型的几何中心
    glm::vec3 COM;
    unsigned int VAO, VBO;
    bool axis_visible;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma) {
        axis_visible = true;
        loadModel(path);
        setUpCom();
    }

    void setUpCom() {
        float vertices[] = {
            COM.x, COM.y, COM.z, 1.0f, 0.0f, 0.0f,
            COM.x + 0.1f, COM.y, COM.z, 1.0f, 0.0f, 0.0f,
            COM.x, COM.y, COM.z, 0.0f, 1.0f, 0.0f,
            COM.x, COM.y + 0.1f, COM.z, 0.0f, 1.0f, 0.0f,
            COM.x, COM.y, COM.z, 0.0f, 0.0f, 1.0f,
            COM.x, COM.y, COM.z + 0.1f, 0.0f, 0.0f, 1.0f,
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

    //绘制物体坐标轴
    void DrawCOM(Shader *shader, Camera *camera, glm::mat4 model, glm::mat4 projection) {
        Model Axis("./statics/xyz_axis.glb");
        shader->use();
        glm::mat4 translation = glm::mat4(1.0f);
        translation = glm::translate(translation, COM);
        translation = glm::rotate(translation, glm::radians(180.0f), glm::vec3(0, 0, 1));
        translation = glm::rotate(translation, glm::radians(-90.0f), glm::vec3(0, 1, 0));
        model = model * translation;
        shader->setMat4("model", model);
        shader->setMat4("view", camera->GetViewMatrix());
        shader->setMat4("projection", projection);

        Axis.Draw(shader);
        // glBindVertexArray(this->VAO);
        // glDrawArrays(GL_LINES, 0, 6);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader *shader) {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path) {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(
            path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }

        // retrieve the directory path of the filepath
        directory = (path.find('/') != string::npos)
                        ? path.substr(0, path.find_last_of('/'))
                        : path.substr(0, path.find_last_of('\\'));
        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
        float COM_X_TOTAL = 0.0f;
        float COM_Y_TOTAL = 0.0f;
        float COM_Z_TOTAL = 0.0f;
        float points = 0;
        for (int i = 0; i < meshes.size(); i++) {
            COM_X_TOTAL += meshes[i].COM_TOTAL.x;
            COM_Y_TOTAL += meshes[i].COM_TOTAL.y;
            COM_Z_TOTAL += meshes[i].COM_TOTAL.z;
            points += meshes[i].vertices.size();
        }
        COM = glm::vec3(COM_X_TOTAL / points, COM_Y_TOTAL / points, COM_Z_TOTAL / points);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene) {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<ModelTexture> textures;
        vector<ModelMaterial> materials;
        float COM_X_TOTAL = 0.0f;
        float COM_Y_TOTAL = 0.0f;
        float COM_Z_TOTAL = 0.0f;
        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            glm::vec3 vector;
            // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // COMs
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            COM_X_TOTAL += vector.x;
            COM_Y_TOTAL += vector.y;
            COM_Z_TOTAL += vector.z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals()) {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            } else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        map<int, string> textureMap;
        // process materials
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        // 此处分支处理 若有内涵材质则读取内涵材质 如glb gltf等格式 则从内存读取文件 若为外置材质文件如obj格式则从文件读取材质
        if (scene->mNumMaterials > 0) {
            materials.emplace_back(loadOriMaterials(material));
        } else if (scene->mNumTextures > 0) {
            vector<ModelTexture> diffuseMaps = loadPackagedTexture(scene, material, aiTextureType_DIFFUSE,
                                                                   "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            vector<ModelTexture> specularMaps = loadPackagedTexture(scene, material, aiTextureType_SPECULAR,
                                                                    "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            // 3. normal maps
            std::vector<ModelTexture> normalMaps = loadPackagedTexture(scene, material, aiTextureType_NORMALS,
                                                                       "texture_normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            // 4. height maps
            std::vector<ModelTexture> heightMaps = loadPackagedTexture(scene, material, aiTextureType_HEIGHT,
                                                                       "texture_height");
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        } else {
            vector<ModelTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            // 2. specular maps
            vector<ModelTexture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR,
                                                                     "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            // 3. normal maps
            std::vector<ModelTexture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS,
                                                                        "texture_normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            // 4. height maps
            std::vector<ModelTexture> heightMaps = loadMaterialTextures(material, aiTextureType_HEIGHT,
                                                                        "texture_height");
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        }

        glm::vec3 COM = glm::vec3(COM_X_TOTAL, COM_Y_TOTAL, COM_Z_TOTAL);
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures, materials, COM);
    }

    ModelMaterial loadOriMaterials(aiMaterial *mat) {
        ModelMaterial material;
        aiColor3D diffuseColor(0.0f);
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == 0) {
            material.diffuseColor = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
        }
        return material;
    }

    // 从内存中读取文件
    vector<ModelTexture> loadPackagedTexture(const aiScene *scene, aiMaterial *mat, aiTextureType type,
                                             string typeName) {
        vector<ModelTexture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            //获取材质index
            string indexstr = str.C_Str();
            indexstr.erase(0, 1);
            int index = stoi(indexstr);
            aiTexture *textureEmbedded = scene->mTextures[index];
            ModelTexture texture;
            texture.id = LoadEmbeddedTexture(textureEmbedded);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
        return textures;
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<ModelTexture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName) {
        vector<ModelTexture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if (!skip) {
                // if texture hasn't been loaded already, load it
                ModelTexture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
                // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
            }
        }
        return textures;
    }
};

unsigned int LoadEmbeddedTexture(const aiTexture *texture) {
    unsigned char *image_data = nullptr;
    int width, height, components_per_pixel;
    if (texture->mHeight == 0) {
        image_data = stbi_load_from_memory(reinterpret_cast<unsigned char *>(texture->pcData), texture->mWidth, &width,
                                           &height, &components_per_pixel, 0);
    } else {
        image_data = stbi_load_from_memory(reinterpret_cast<unsigned char *>(texture->pcData),
                                           texture->mWidth * texture->mHeight, &width, &height, &components_per_pixel,
                                           0);
    }
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    if (components_per_pixel == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
    } else if (components_per_pixel == 4) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    }
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return textureID;
}

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma) {
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
#endif
