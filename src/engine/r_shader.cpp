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
#include "r_shader.h"
#include "filesystem.h"
#include "console.h"

R_Shader::R_Shader()
{
}

R_Shader::~R_Shader()
{
    if (bgfx::isValid(m_program))
    {
        bgfx::destroy(m_program);
    }
}

static std::string GetShaderPlatformPath()
{
    switch (bgfx::getRendererType())
    {
    case bgfx::RendererType::Noop:
        return "noop";
    case bgfx::RendererType::Vulkan:
        return "vulkan";
    default:
        return "vulkan";
    }
}

bgfx::ShaderHandle R_Shader::LoadShaderBinary(const std::string& path)
{
    std::string fileName = path;
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        std::string dir = path.substr(0, lastSlash);
        std::string file = path.substr(lastSlash + 1);
        fileName = dir + "/" + GetShaderPlatformPath() + "/" + file;
    }

    auto data = Filesystem::ReadBinary(fileName);
    if (data.empty())
    {
        return BGFX_INVALID_HANDLE;
    }

    const bgfx::Memory* mem = bgfx::copy(data.data(), (uint32_t)data.size());
    return bgfx::createShader(mem);
}

bool R_Shader::Load(const std::string& vertPath, const std::string& fragPath)
{
    bgfx::ShaderHandle vsh = LoadShaderBinary(vertPath + ".bin");
    bgfx::ShaderHandle fsh = LoadShaderBinary(fragPath + ".bin");

    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh))
    {
        Console::Error("Failed to load shader binaries for " + vertPath);
        return false;
    }

    m_program = bgfx::createProgram(vsh, fsh, true);
    return bgfx::isValid(m_program);
}

bool R_Shader::LoadCompute(const std::string& path)
{
    bgfx::ShaderHandle csh = LoadShaderBinary(path + ".bin");

    if (!bgfx::isValid(csh))
    {
        Console::Error("Failed to load compute shader binary for " + path);
        return false;
    }

    m_program = bgfx::createProgram(csh, true);
    return bgfx::isValid(m_program);
}