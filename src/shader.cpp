/*
 * MIT License
 *
 * Copyright (c) 2025-2026 Soft Sprint Studios
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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
    const char* version = "#version 460 core\n";
    const char* lineReset = "#line 1\n";
    const char* src = source.c_str();

    const char* sources[] = { version, lineReset, src };
    glShaderSource(shader, 3, sources, NULL);
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

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(GetUniformLocation(name), value);
}