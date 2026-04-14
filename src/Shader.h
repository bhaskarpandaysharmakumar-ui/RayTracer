#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexShaderPath, const char* fragmentShaderPath);
    void Bind() const;
    void Unbind() const;
    void SetBool(const std::string &name, bool value) const;
    void SetInt(const std::string &name, int value) const;
    void SetFloat(const std::string &name, float value) const;
    void SetVec3(const std::string &name, glm::vec3 &value) const;
    void SetMat4(const std::string &name, glm::mat4 &value) const;
};
