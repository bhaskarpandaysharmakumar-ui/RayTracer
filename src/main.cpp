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

struct AABB {
    glm::vec2 x;
    alignas(8) glm::vec2 y;
    alignas(8) glm::vec2 z;

    bool operator==(const AABB& a) const {
        return a.x == x && a.y == y && a.z == z;
    }
};

const AABB EMPTYBBOX = {{0, 0}, {0, 0}, {0, 0}};

#define DIFFUSE 0
#define METAL 1
#define DIELECTRIC 2

struct Node {
    AABB bbox;

    alignas(16) glm::vec3 sPos; // note that only leaf nodes would have a sensible position and radius
    float sRadius;

    alignas(16) glm::vec3 attenuation;
    int type;
    float refractiveIndex;

    int parent = -1;
    int lChild = -1;
    int rChild = -1;
};

bool BoxCompareX(const Node& a, const Node& b) {
    return a.bbox.x.x < b.bbox.x.x;
}

bool BoxCompareY(const Node& a, const Node& b) {
    return a.bbox.y.x < b.bbox.y.x;
}

bool BoxCompareZ(const Node& a, const Node& b) {
    return a.bbox.z.x < b.bbox.z.x;
}

AABB ConstructBBox(AABB& b1, AABB& b2) {
    if (b1 == EMPTYBBOX) return b2;
    if (b2 == EMPTYBBOX) return b1;

    AABB box = EMPTYBBOX;
    box.x[0] = b1.x[0] < b2.x[0] ? b1.x[0] : b2.x[0];
    box.x[1] = b1.x[1] > b2.x[1] ? b1.x[1] : b2.x[1];
    box.y[0] = b1.y[0] < b2.y[0] ? b1.y[0] : b2.y[0];
    box.y[1] = b1.y[1] > b2.y[1] ? b1.y[1] : b2.y[1];
    box.z[0] = b1.z[0] < b2.z[0] ? b1.z[0] : b2.z[0];
    box.z[1] = b1.z[1] > b2.z[1] ? b1.z[1] : b2.z[1];
    return box;
}

float LargestNum(float x, float y, float z) {
    return (x > y ? (x > z ? x : z) : (y > z ? y : z));
}

int LongestAxis(const AABB& b) {
    float n = LargestNum(glm::abs(b.x.y - b.x.x), glm::abs(b.y.y - b.y.x), glm::abs(b.z.y - b.z.x));
    if (n == glm::abs(b.x.y - b.x.x)) return 0;
    if (n == glm::abs(b.y.y - b.y.x)) return 1;
    return 2;
}

// Constructs a node with a bbox that tightly encloses all the nodes (bboxes) in the given list
Node ConstructNodeFromList(std::vector<Node>& objects, size_t start, size_t end) {
    AABB box = EMPTYBBOX;
    glm::vec3 pos(69.f);
    float radius = -1.f;
    glm::vec3 atten(0.f);
    int type = -1;
    float ri=-1.f;

    for (int i = (int)start; i < end; ++i) {
        box = ConstructBBox(box, objects[i].bbox);
        pos = objects[i].sPos;
        radius = objects[i].sRadius;
        atten = objects[i].attenuation;
        type = objects[i].type;
        ri = objects[i].refractiveIndex;
    }

    return { box, pos, radius, atten, type, ri, -1, -1, -1 };
}

void ConstructBVH(std::vector<Node>& objects, size_t start, size_t end, std::vector<Node>& tree) {
    static int parentIndex = 0;

    AABB bbox = EMPTYBBOX;
    for (int i = (int)start; i < end; ++i) bbox = ConstructBBox(bbox, objects[i].bbox);
    int axis = LongestAxis(bbox);
    auto comparator = (axis == 0 ? BoxCompareX : (axis == 1 ? BoxCompareY : BoxCompareZ));

    std::sort(std::begin(objects) + (int)start, std::begin(objects) + (int)end, comparator);

    size_t mid = start + (end-start)/2;
    size_t mid2 = start + (mid-start)/2 + 1;

    int currentNodeIndexL = (int)tree.size();

    Node leftNode = ConstructNodeFromList(objects, start, mid);
    leftNode.parent = parentIndex;
    if (parentIndex > 0) tree[parentIndex].lChild = currentNodeIndexL;
    tree.push_back(leftNode);

    Node rightNode = ConstructNodeFromList(objects, mid, end);
    rightNode.parent = parentIndex;
    if (parentIndex > 0) tree[parentIndex].rChild = currentNodeIndexL+1;
    tree.push_back(rightNode);

    bool splitLeftNode = mid2 - start > 1;
    bool splitRightNode = end - mid2 > 1;
    if (splitLeftNode) {
        parentIndex = currentNodeIndexL;
        ConstructBVH(objects, start, mid2, tree);
    }
    if (splitRightNode) {
        parentIndex = currentNodeIndexL+1;
        ConstructBVH(objects, mid2, end, tree);
    }
}

AABB ConstructAABB_Sphere(glm::vec3 pos, float radius) {
    AABB bbox{};
    glm::vec3 p = pos;
    float r = radius;
    bbox.x = p[0]-r > p[0]+r ? glm::vec2(p[0]+r, p[0]-r) : glm::vec2(p[0]-r, p[0]+r);
    bbox.y = p[1]-r > p[1]+r ? glm::vec2(p[1]+r, p[1]-r) : glm::vec2(p[1]-r, p[1]+r);
    bbox.z = p[2]-r > p[2]+r ? glm::vec2(p[2]+r, p[2]-r) : glm::vec2(p[2]-r, p[2]+r);

    return bbox;
}

void PrintTree(std::vector<Node>& tree) {
    for (Node& node : tree) {
        printf("{\n");
        printf("\tInterval x: %lf, %f\n", node.bbox.x.x, node.bbox.x.y);
        printf("\tInterval y: %lf, %f\n", node.bbox.y.x, node.bbox.y.y);
        printf("\tInterval z: %lf, %f\n", node.bbox.z.x, node.bbox.z.y);
        // printf("\tRefractiveIndex: %lf\n", node.refractiveIndex);
        // printf("\tType: %d\n", node.type);
        printf("\tRadius: %f\n", node.sRadius);
        printf("\tParent: %d\n", node.parent);
        printf("\tlChild: %d\n", node.lChild);
        printf("\trChild: %d\n", node.rChild);
        printf("}\n");
    }
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

    std::vector<Node> objects, tree;
    objects.push_back({ConstructAABB_Sphere({0.f, 0.0f, -.7f}, .3f), {0.f, 0.0f, -.7f}, .3f,
                                                                {0.1, 0.2, 0.5}, DIFFUSE, 0.f});
    objects.push_back({ConstructAABB_Sphere({0.f, -300.301f, -.7f}, 300.f), {0.f, -300.301f, -.7f}, 300.f,
                                                                {0.8, 0.8, 0.0}, DIFFUSE, 0.f});
    objects.push_back({ConstructAABB_Sphere({-0.62f, 0.0f, -.7f}, .3f), {-0.62f, 0.0f, -.7f}, .3f,
                                                                    {0.8, 0.8, 0.8}, METAL, 0.f});
    objects.push_back({ConstructAABB_Sphere({.62f, 0.0f, -.7f}, .3f), {.62f, 0.0f, -.7f}, .3f,
                                                                    {0, 0, 0}, DIELECTRIC, 1.33f});
    objects.push_back({ConstructAABB_Sphere({.62f, 0.0f, -.7f}, .3f - 0.05f), {.62f, 0.0f, -.7f}, .3f-0.05f,
                                                                    {0, 0, 0}, DIELECTRIC, 1.f/1.33f}); // the air sphere

    Node root = ConstructNodeFromList(objects, 0, objects.size());
    root.parent = -1;
    root.lChild = 1;
    root.rChild = 2;
    tree.push_back(root);

    ConstructBVH(objects, 0, objects.size(), tree);

    // PrintTree(tree);

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

    unsigned int ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)(tree.size() * sizeof(Node)), tree.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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

        glm::mat4 viewRot = glm::mat4(1.f);
        glm::mat4 viewTrans = glm::mat4(1.f);
        viewRot = glm::rotate(viewRot, glm::radians(pitch), {1, 0, 0});
        viewRot = glm::rotate(viewRot, glm::radians(yaw), {0, 1, 0});
        viewTrans = glm::translate(viewTrans, -pos);

        glm::mat4 view = viewTrans * viewRot;

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

        computeShader.Bind();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo);
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