#include "shader.h"
#include "filesystem.h"
#include "console.h"
#include <glm/gtc/type_ptr.hpp>

Shader::Shader() : m_program(0)
{
}

Shader::~Shader()
{
    if (m_program != 0)
    {
        glDeleteProgram(m_program);
    }
}

bool Shader::Load(const std::string& vertPath, const std::string& fragPath)
{
    std::string vCode = Filesystem::ReadText(vertPath);
    std::string fCode = Filesystem::ReadText(fragPath);

    if (vCode.empty() || fCode.empty())
    {
        return false;
    }

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vCode);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fCode);

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, NULL, infoLog);
        Console::Error("Shader Link Error: " + std::string(infoLog));
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
}

GLuint Shader::CompileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        Console::Error("Shader Compile Error: " + std::string(infoLog));
    }
    return shader;
}

void Shader::Bind() const
{
    glUseProgram(m_program);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

GLint Shader::GetUniformLocation(const std::string& name) const
{
    if (m_uniformLocationCache.find(name) != m_uniformLocationCache.end())
    {
        return m_uniformLocationCache[name];
    }

    GLint location = glGetUniformLocation(m_program, name.c_str());
    m_uniformLocationCache[name] = location;
    return location;
}

void Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetVec3(const std::string& name, const glm::vec3& vec) const
{
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(GetUniformLocation(name), value);
}