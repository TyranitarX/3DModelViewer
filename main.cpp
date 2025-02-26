#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif

#include <iostream>
#include <fstream>
#include "light.h"
#include "shader.h"
#include <vector>

#include "camera.h"
#include "framebuffer.h"
#include "model.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h";
#include "imgui/imgui_impl_opengl3.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"


using namespace std;


void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void processInput(GLFWwindow *window, Camera *camera);

void mouseCallBack(GLFWwindow *window, double xpos, double ypos);

void scrollCallBack(GLFWwindow *window, double xoffset, double yoffset);

GLFWwindow *initOpenGL(Camera *camera);

void setupMesh();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float deltaTime = 0.0f; // 当前帧与上一帧的时间差
float lastFrame = 0.0f; // 上一帧的时间


int main() {
    //相机作为观察矩阵 不每次初始化
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    GLFWwindow *window = initOpenGL(&camera);
    if (!window)
        return -1;
    Shader lightShader("./shaders/lightvs.glsl", "./shaders/lightfs.glsl");
    Shader modelShader("./shaders/modelvs.glsl", "./shaders/modelfs.glsl");
    Shader glbShader("./shaders/lightvs.glsl", "./shaders/axisfs.glsl");
    //光照属性
    glm::vec3 lightAmbient(0.2f, 0.2f, 0.2f);
    glm::vec3 lightDiffuse(0.5f, 0.5f, 0.5f);
    glm::vec3 lightSpecular(1.0f, 1.0f, 1.0f);
    //点光源位置
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    //手电筒光线角度
    float fai = 12.5f;
    float gama = 17.5f;
    vector<Model> loadedmodels;
    vector<glm::vec3> modelPos;
    vector<glm::vec3> modelRotate;
    vector<float> modelScale;
    vector<Light> lights;
    vector<Camera> cameras;
    bool model_ToolActivate = false;
    bool light_ToolActivate = false;
    float backGroundColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    int renderType = GL_FILL;
    //点光源
    glm::vec4 lightViewPos = camera.GetViewMatrix() * glm::vec4(lightPos, 1.0f);
    glm::vec3 spotDir = glm::normalize(
        glm::vec3(lightViewPos) - glm::vec3(camera.GetViewMatrix() * glm::vec4(0.0, 0.0, 0.0, 1.0)));
    PointLight pointlight(0, glm::vec3(lightViewPos), spotDir, lightAmbient, lightDiffuse, lightSpecular,
                          1.0f, 0.09f, 0.032f);
    DirectionalLight directionallight(1, glm::vec3(-0.2f, -1.0f, -0.3f), lightAmbient,
                                      glm::vec3(1.0f, 1.0f, 1.0f),
                                      lightSpecular);
    lights.emplace_back(pointlight);
    lights.emplace_back(directionallight);
    ImVec2 wsize = ImVec2(SCR_WIDTH, SCR_HEIGHT);
    //每一帧渲染界面
    while (!glfwWindowShouldClose(window)) {
        Framebuffer fb_framebuffer(wsize.x, wsize.y);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f,
                                                100.0f);
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }
        // // //Z缓冲 保证透视正常
        //
        // glClearColor(backGroundColor[0], backGroundColor[1], backGroundColor[2], backGroundColor[3]);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // 绑定对应的帧缓冲 绘制图像
        glBindFramebuffer(GL_FRAMEBUFFER, fb_framebuffer.framebuffer);
        glClearColor(backGroundColor[0], backGroundColor[1], backGroundColor[2], backGroundColor[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 我们现在不使用模板缓冲
        glEnable(GL_DEPTH_TEST);
        for (int i = 0; i < loadedmodels.size(); i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(modelScale[i]));
            model = glm::translate(model, modelPos[i]);
            model = glm::translate(model, loadedmodels[i].COM);
            model = glm::rotate(model, glm::radians(modelRotate[i].z),
                                glm::vec3(0.f, 0.f, 1.f));
            model = glm::rotate(model, glm::radians(modelRotate[i].x),
                                glm::vec3(1.f, 0.f, 0.f));
            model = glm::rotate(model, glm::radians(modelRotate[i].y),
                                glm::vec3(0.f, 1.f, 0.f));
            model = glm::translate(model, -loadedmodels[i].COM);
            modelShader.use();
            modelShader.setInt("lightNumbers", 2);
            directionallight.addToShader(&modelShader);
            pointlight.addToShader(&modelShader);
            glm::mat3 NormalMatrix = glm::mat3(glm::transpose(glm::inverse(camera.GetViewMatrix() * model)));
            modelShader.setMat3("normalMatrix", NormalMatrix);
            modelShader.setMat4("viewMatrix", camera.GetViewMatrix() * model);
            modelShader.setMat4("transMatrix", projection * camera.GetViewMatrix() * model);
            glPolygonMode(GL_FRONT_AND_BACK, renderType);
            glDepthFunc(GL_LESS);
            loadedmodels[i].Draw(&modelShader);
            if (loadedmodels[i].axis_visible) {
                glDepthFunc(GL_ALWAYS);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                loadedmodels[i].DrawCOM(&glbShader, &camera, model, projection);
            }
            // glDisable(GL_CULL_FACE);
        }
        // input
        // -----
        processInput(window, &camera);
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        pointlight.Draw(&lightShader, &camera, projection);

        //返回主窗口的帧缓冲 绘制界面
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(backGroundColor[0], backGroundColor[1], backGroundColor[2], backGroundColor[3]);
        glClear(GL_COLOR_BUFFER_BIT); // 我们现在不使用模板缓冲
        glEnable(GL_DEPTH_TEST);
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        //Menu Bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("import Models", "Ctrl+O")) {
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".obj,.glb,.stl,.ply");
                }
                if (ImGui::MenuItem("close", "Ctrl+W")) { model_ToolActivate = false; }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Windows")) {
                if (ImGui::MenuItem("Light Tools")) {
                    if (!light_ToolActivate) {
                        light_ToolActivate = true;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        //Scene Window
        ImGui::Begin("Scene");
        wsize = ImGui::GetWindowSize();
        ImGui::Image((ImTextureID) fb_framebuffer.texColorBuffer, ImVec2(wsize.x, wsize.y), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::End();
        //light window
        if (light_ToolActivate) {
            // 创建 ImGui 窗口和按钮等
            ImGui::Begin("Light Tools", &light_ToolActivate);
            ImGui::Text("light Pos");
            ImGui::SliderFloat("position.x", &pointlight.position.x, -5.0f, 5.0f);
            ImGui::SliderFloat("position.y", &pointlight.position.y, -5.0f, 5.0f);
            ImGui::SliderFloat("position.z", &pointlight.position.z, -5.0f, 5.0f);
            ImGui::Text("Light Config");
            ImGui::SliderFloat("lightAmbient.x", &lightAmbient.x, 0.0f, 1.0f);
            ImGui::SliderFloat("lightAmbient.y", &lightAmbient.y, 0.0f, 1.0f);
            ImGui::SliderFloat("lightAmbient.z", &lightAmbient.z, 0.0f, 1.0f);
            ImGui::SliderFloat("lightDiffuse.x", &lightDiffuse.x, 0.0f, 5.0f);
            ImGui::SliderFloat("lightDiffuse.y", &lightDiffuse.y, 0.0f, 5.0f);
            ImGui::SliderFloat("lightDiffuse.z", &lightDiffuse.z, 0.0f, 5.0f);
            ImGui::Text("SpotLight Config");
            ImGui::SliderFloat("fai", &fai, 0.0f, 89.9f);
            ImGui::SliderFloat("gama", &gama, fai, 89.9f);
            ImGui::End();
        }
        //Main Window
        ImGui::Begin("Main", &model_ToolActivate);
        ImGui::Text("BackGroundColor:");
        ImGui::ColorEdit4("Color", &backGroundColor[0]);
        ImGui::Text("RenderMethod:");
        ImGui::RadioButton("FILL", &renderType, GL_FILL);
        ImGui::RadioButton("LINE", &renderType, GL_LINE);
        ImGui::RadioButton("POINT", &renderType, GL_POINT);
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                Model newModel(filePathName);
                loadedmodels.push_back(newModel);
                modelPos.emplace_back(0.0f, 0.0f, 0.0f);
                modelRotate.emplace_back(0.0f, 0.0f, 0.0f);
                modelScale.emplace_back(1.0f);
            }
            ImGuiFileDialog::Instance()->Close();
        }
        if (ImGui::CollapsingHeader("Models")) {
            for (unsigned int i = 0; i < loadedmodels.size(); i++) {
                if (ImGui::TreeNode(("Model" + std::to_string(i)).c_str())) {
                    ImGui::Text("Properties");
                    ImGui::Checkbox("Enable Axis", &loadedmodels[i].axis_visible);
                    ImGui::Text("Model Pos");
                    ImGui::SliderFloat("xPos", &modelPos[i].x, -5.0f, 5.0f);
                    ImGui::SliderFloat("yPos", &modelPos[i].y, -5.0f, 5.0f);
                    ImGui::SliderFloat("zPos", &modelPos[i].z, -5.0f, 5.0f);
                    ImGui::Text("Model Rotate");
                    ImGui::SliderFloat("xRotate", &modelRotate[i].x, -180.0f, 180.0f);
                    ImGui::SliderFloat("yRotate", &modelRotate[i].y, -180.0f, 180.0f);
                    ImGui::SliderFloat("zRotate", &modelRotate[i].z, -180.0f, 180.0f);
                    ImGui::SliderFloat("scale", &modelScale[i], -100.0f, 100.0f);
                    if (ImGui::Button("delete Model")) {
                        loadedmodels.erase(loadedmodels.begin() + i);
                        modelPos.erase(modelPos.begin() + i);
                        modelRotate.erase(modelRotate.begin() + i);
                        modelScale.erase(modelScale.begin() + i);
                    }
                    ImGui::TreePop();
                }
            }
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        fb_framebuffer.destory();
        glfwSwapBuffers(window);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

GLFWwindow *initOpenGL(Camera *camera) {
    glfwInit();
    const char *glsl_version = "#version 460";
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ImGui GLFW3", NULL, NULL);
    if (window == NULL) {
        cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << std::endl;
        return NULL;
    }
    //将glfw捕捉鼠标事件放在imgui初始化之前 保证交互正常
    glfwSetWindowUserPointer(window, camera);
    glfwSetCursorPosCallback(window, mouseCallBack);
    glfwSetScrollCallback(window, scrollCallBack);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return window;
}

void processInput(GLFWwindow *window, Camera *camera) {
    float speed = 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed = 5.0f;
    float cameraSpeed = speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera->ProcessKeyboard(FORWARD, cameraSpeed);
    } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera->ProcessKeyboard(BACKWARD, cameraSpeed);
    } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera->ProcessKeyboard(LEFT, cameraSpeed);
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera->ProcessKeyboard(RIGHT, cameraSpeed);
    }
}

bool firstMouse = true;
bool rightMouseDown = false;
float lastx = 400;
float lasty = 300;

void mouseCallBack(GLFWwindow *window, double xpos, double ypos) {
    Camera *camera = (Camera *) glfwGetWindowUserPointer(window);
    if (firstMouse) {
        lastx = xpos;
        lasty = ypos;
        firstMouse = false;
    }
    if (rightMouseDown) {
        float xoffset = xpos - lastx;
        float yoffset = lasty - ypos;
        lastx = xpos;
        lasty = ypos;

        float sensitivity = 0.05f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;
        camera->ProcessMouseMovement(xoffset, yoffset);
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        rightMouseDown = true;
        lastx = xpos;
        lasty = ypos;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE) {
        rightMouseDown = false;
    }
}


void scrollCallBack(GLFWwindow *window, double xoffset, double yoffset) {
    cout << "scroll!" << endl;
    cout << xoffset << endl;
    cout << yoffset << endl;
    Camera *camera = (Camera *) glfwGetWindowUserPointer(window);
    camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}
