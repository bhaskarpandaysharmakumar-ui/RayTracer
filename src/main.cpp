#include <fstream>
#include <iostream>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.inl>

#include "Shader.h"
#include "input/KeyboardInput.h"
#include "input/MouseInput.h"

class ComputeShader {
public:
    unsigned int ID;

    ComputeShader(const char* shaderPath) {
        std::string shaderCode;
        std::ifstream shaderFile;

        try {
            shaderFile.open(shaderPath);

            std::stringstream shaderStream;

            shaderStream << shaderFile.rdbuf();

            shaderFile.close();

            shaderCode = shaderStream.str();
        } catch (std::ifstream::failure& e) {
            std::cout << "Failed to read compute shader\n";
        }

        const char* source = shaderCode.c_str();

        unsigned int compute;
        int success;
        char infoLog[512];

        compute = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compute, 1, &source, 0);
        glCompileShader(compute);
        glGetShaderiv(compute, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(compute, 512, NULL, infoLog);
            std::cout << "Compute Shader compilation failed: \n" << infoLog << std::endl;
        }

        ID = glCreateProgram();
        glAttachShader(ID, compute);
        glLinkProgram(ID);
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success)
        {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cout << "Compute Shader program linking failed: \n" << infoLog << std::endl;
        }
    }

    void Bind() const {
        glUseProgram(ID);
    }

    void Unbind() const {
        glUseProgram(0);
    }

    void SetFloat(const std::string &name, float value) const {
        Bind();
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void SetMat4(const std::string &name, glm::mat4 &value) const {
        Bind();
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
    }

    void SetVec3(const std::string &name, glm::vec3 &value) const {
        Bind();
        glUniform3f(glGetUniformLocation(ID, name.c_str()), value.x, value.y, value.z);
    }
};

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, 9.f / 16.f * width);
}


int main() {
    if (!glfwInit())
        std::cout << "Failed to initialize glfw\n";

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "RayTracy", nullptr, nullptr);
    if (!window)
        std::cout << "Failed to create glfw window\n";

    glfwMakeContextCurrent(window);
    gladLoadGL();

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, MouseInput::CursorPosCallback);
    glfwSetKeyCallback(window, KeyboardInput::KeyCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    float vertices[] = {
        -0.5f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
    };

    unsigned int texCoords[] = {
        0, 1,
        0, 0,
        1, 1,
        1, 1,
        1, 0,
        0, 0,
    };

    unsigned int vao, vbo, tbo;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &tbo);
    glBindBuffer(GL_ARRAY_BUFFER, tbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_UNSIGNED_INT, GL_FALSE, 2 * sizeof(unsigned int), 0);
    glEnableVertexAttribArray(1);

    Shader shader("../res/vertex.glsl", "../res/fragment.glsl");

    glm::mat4 proj = glm::ortho(0.f, 1280.f, 0.f, 720.f, -1.f, 1.f);
    shader.SetMat4("proj", proj);

    ComputeShader computeShader("../res/main.comp");

    const unsigned int TEXTURE_WIDTH = 1280, TEXTURE_HEIGHT = 720;
    unsigned int texture;

    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

    glm::vec3 pos(0.0);
    float pitch, yaw;
    float sens = 0.08;
    float speed = 1.5f;

    float dt = 0.f, prevTime = 0.f;
    while (!glfwWindowShouldClose(window)) {
        // glEnable(GL_FRAMEBUFFER_SRGB);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0, 0, 0, 1);

        float currentTime = (float)glfwGetTime();
        dt = currentTime - prevTime;

        pitch += MouseInput::GetOffsetY() * sens;
        yaw -= MouseInput::GetOffsetX() * sens;

        auto view = glm::mat4(1.f);
        // view = glm::rotate(view, -glm::radians(pitch), {1, 0, 0});
        // view = glm::rotate(view, glm::radians(yaw), {0, 1, 0});
        // view = glm::translate(view, -pos);
        glm::mat4 viewRot = glm::mat4(1.f);
        glm::mat4 viewTrans = glm::mat4(1.f);
        viewRot = glm::rotate(viewRot, glm::radians(pitch), {1, 0, 0});
        //std::cout << pitch << "\n";
        viewRot = glm::rotate(viewRot, glm::radians(yaw), {0, 1, 0});
        viewTrans = glm::translate(viewTrans, -pos);

        view = viewTrans * viewRot;

        auto inv = glm::inverse(view);
        glm::vec3 forward = {inv[0][2], inv[1][2], inv[2][2]};
        glm::vec3 left = {inv[0][0], inv[1][0], inv[2][0]};
        glm::vec3 up = {view[0][1], view[1][1], view[2][1]};

        if (KeyboardInput::KeyPressed(GLFW_KEY_W))
            pos += speed * forward * dt;
        if (KeyboardInput::KeyPressed(GLFW_KEY_S))
            pos -= speed * forward * dt;
        if (KeyboardInput::KeyPressed(GLFW_KEY_A))
            pos += speed * left * dt;
        if (KeyboardInput::KeyPressed(GLFW_KEY_D))
            pos -= speed * left * dt;
        if (KeyboardInput::KeyPressed(GLFW_KEY_E))
            pos -= speed * glm::vec3(0, 1, 0) * dt;
        if (KeyboardInput::KeyPressed(GLFW_KEY_Q))
            pos += speed * glm::vec3(0, 1, 0) * dt;

        //view = glm::lookAt(glm::vec3(), glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 1.f, 0.f));

        computeShader.Bind();
        // computeShader.SetMat4("viewMat", view);
        computeShader.SetMat4("viewRot", viewRot);
        computeShader.SetMat4("viewTrans", viewTrans);
        computeShader.SetVec3("camPos", pos);
        computeShader.SetVec3("camForward", forward);
        glDispatchCompute(TEXTURE_WIDTH/10, TEXTURE_HEIGHT/10, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        shader.Bind();

        glm::mat4 model = glm::identity<glm::mat4>();
        model = glm::translate(model, {640, 360, 0});
        model = glm::scale(model, {TEXTURE_WIDTH, TEXTURE_HEIGHT, 1});
        shader.SetMat4("model", model);

        shader.SetInt("tex", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // std::cout << dt << "\n";

        glBindVertexArray(vao);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices) / sizeof(float));
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindVertexArray(0);

        shader.Unbind();

        prevTime = currentTime;

        MouseInput::EndFrame();
        KeyboardInput::EndFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}