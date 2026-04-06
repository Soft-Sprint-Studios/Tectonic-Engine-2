#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader
{
public:
    Shader();
    ~Shader();

    bool Load(const std::string& vertPath, const std::string& fragPath);
    void Bind() const;
    void Unbind() const;

    void SetMat4(const std::string& name, const glm::mat4& mat) const;
    void SetVec3(const std::string& name, const glm::vec3& vec) const;
    void SetInt(const std::string& name, int value) const;

private:
    GLint GetUniformLocation(const std::string& name) const;

    GLuint m_program;
    mutable std::unordered_map<std::string, GLint> m_uniformLocationCache;
    GLuint CompileShader(GLenum type, const std::string& source);
};