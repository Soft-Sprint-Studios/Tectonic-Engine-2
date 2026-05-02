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
#include "concmd.h"
#include "r_shader.h"
#include "filesystem.h"
#include "console.h"
#include <glm/gtc/type_ptr.hpp>

R_Shader::R_Shader() : m_program(0)
{
    GetRegistry().insert(this);
}

R_Shader::~R_Shader()
{
    GetRegistry().erase(this);

    if (m_program != 0)
    {
        glDeleteProgram(m_program);
    }
}

bool R_Shader::Load(const std::string& vertPath, const std::string& fragPath, const std::string& geomPath)
{
    // We must store these for reloading shaders
    m_vertPath = vertPath;
    m_fragPath = fragPath;
    m_geomPath = geomPath;
    m_computePath = "";

    std::string vCode = Filesystem::ReadText(vertPath);
    std::string fCode = Filesystem::ReadText(fragPath);
    if (vCode.empty() || fCode.empty())
    {
        return false;
    }

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vCode, vertPath);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fCode, fragPath);
    GLuint gs = 0;

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);

    if (!geomPath.empty())
    {
        std::string gCode = Filesystem::ReadText(geomPath);
        if (!gCode.empty())
        {
            gs = CompileShader(GL_GEOMETRY_SHADER, gCode, geomPath);
            glAttachShader(m_program, gs);
        }
    }

    glLinkProgram(m_program);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, NULL, infoLog);
        std::string paths = vertPath + " | " + fragPath;
        Console::Error("Shader Link Error [" + paths + "]:\n" + std::string(infoLog));
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    if (gs != 0)
    {
        glDeleteShader(gs);
    }

    return true;
}

bool R_Shader::LoadCompute(const std::string& path)
{
    // We must store these for reloading shaders
    m_computePath = path;
    m_vertPath = m_fragPath = m_geomPath = "";

    std::string code = Filesystem::ReadText(path);
    if (code.empty())
    {
        return false;
    }

    GLuint cs = CompileShader(GL_COMPUTE_SHADER, code, path);
    m_program = glCreateProgram();
    glAttachShader(m_program, cs);
    glLinkProgram(m_program);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, NULL, infoLog);
        Console::Error("Compute Shader Link Error [" + path + "]:\n" + std::string(infoLog));
        return false;
    }

    glDeleteShader(cs);
    return true;
}

GLuint R_Shader::CompileShader(GLenum type, const std::string& source, const std::string& path)
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
        Console::Error("Shader Compile Error in [" + path + "]:\n" + std::string(infoLog));
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool R_Shader::Reload()
{
    if (m_program != 0)
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    m_uniformLocationCache.clear();

    if (!m_computePath.empty())
    {
        return LoadCompute(m_computePath);
    }
    else if (!m_vertPath.empty())
    {
        return Load(m_vertPath, m_fragPath, m_geomPath);
    }

    return false;
}

void R_Shader::ReloadAll()
{
    for (R_Shader* shader : GetRegistry())
    {
        shader->Reload();
    }
}

void R_Shader::Bind() const
{
    glUseProgram(m_program);
}

void R_Shader::Unbind() const
{
    glUseProgram(0);
}

GLint R_Shader::GetUniformLocation(const std::string& name) const
{
    if (m_uniformLocationCache.find(name) != m_uniformLocationCache.end())
    {
        return m_uniformLocationCache[name];
    }

    GLint location = glGetUniformLocation(m_program, name.c_str());
    m_uniformLocationCache[name] = location;
    return location;
}

void R_Shader::SetMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void R_Shader::SetVec4(const std::string& name, const glm::vec4& vec) const
{
    glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

void R_Shader::SetVec3(const std::string& name, const glm::vec3& vec) const
{
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

void R_Shader::SetVec2(const std::string& name, const glm::vec2& vec) const
{
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(vec));
}

void R_Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(GetUniformLocation(name), value);
}

void R_Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(GetUniformLocation(name), value);
}

std::set<R_Shader*>& R_Shader::GetRegistry()
{
    static std::set<R_Shader*> s_registry;
    return s_registry;
}

CON_COMMAND(shaders_reload, "Reloads all shaders from disk.")
{
    R_Shader::ReloadAll();
    Console::Log("All shaders reloaded.");
}