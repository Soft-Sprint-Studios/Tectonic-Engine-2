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
#include "r_sky.h"
#include "cvar.h"
#include "timing.h"
#include "dynamic_sky.h"
#include "filesystem.h"
#include "dds.h"
#include <glm/gtc/type_ptr.hpp>

CVar r_sky_steps_primary("r_sky_steps_primary", "8", "Primary raymarching steps for dynamic sky.", CVAR_SAVE);
CVar r_sky_steps_light("r_sky_steps_light", "3", "Secondary light-sampling steps for dynamic sky.", CVAR_SAVE);

R_Sky::R_Sky() 
{
}

R_Sky::~R_Sky() 
{ 
    Shutdown(); 
}

bool R_Sky::Init(const std::string& skyName)
{
    m_shader.Load("shaders/sky.vert", "shaders/sky.frag");

    m_layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .end();

    float skyboxVertices[] =
    {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f
    };

    m_vbo = bgfx::createVertexBuffer(bgfx::copy(skyboxVertices, sizeof(skyboxVertices)), m_layout);

    m_sSkybox = bgfx::createUniform("s_skybox", bgfx::UniformType::Sampler);
    m_uSkyParams = bgfx::createUniform("u_skyParams", bgfx::UniformType::Vec4);
    m_uSunDir = bgfx::createUniform("u_sunDir", bgfx::UniformType::Vec4);
    m_uSunColor = bgfx::createUniform("u_sunColor", bgfx::UniformType::Vec4);
    m_uViewPos = bgfx::createUniform("u_viewPos", bgfx::UniformType::Vec4);
    m_uTime = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);

    LoadCubemap(skyName);
    return true;
}

void R_Sky::LoadCubemap(const std::string& skyName)
{
    if (bgfx::isValid(m_cubemapTexture))
    {
        bgfx::destroy(m_cubemapTexture);
    }

    std::vector<std::string> faces = { "right", "left", "top", "bottom", "front", "back" };
    std::vector<uint8_t> allData;
    uint32_t width = 0, height = 0;

    for (unsigned int i = 0; i < 6; i++)
    {
        std::string path = "textures/skybox/" + skyName + "_" + faces[i] + ".dds";
        DDS::ImageInfo info;
        if (DDS::Load(path, true, info) && !info.mips.empty())
        {
            width = info.width;
            height = info.height;
            allData.insert(allData.end(), info.data.begin() + info.mips[0].offset, info.data.begin() + info.mips[0].offset + info.mips[0].size);
        }
        else
        {
            width = height = 512;
            std::vector<uint8_t> black(512 * 512 * 4, 0);
            allData.insert(allData.end(), black.begin(), black.end());
        }
    }

    const bgfx::Memory* mem = bgfx::copy(allData.data(), (uint32_t)allData.size());
    m_cubemapTexture = bgfx::createTextureCube((uint16_t)width, false, 1, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP, mem);
}

void R_Sky::Draw(bgfx::ViewId viewId, const Camera& camera)
{
    const auto& sky = DynamicSky::GetSettings();

    float skyParams[4] = 
    { 
        sky.useDynamic ? 1.0f : 0.0f, 
        (float)r_sky_steps_primary.GetInt(), 
        (float)r_sky_steps_light.GetInt(), 
        0.0f 
    };
    bgfx::setUniform(m_uSkyParams, skyParams);

    float sunDir[4] = { sky.sunDir.x, sky.sunDir.y, sky.sunDir.z, 0.0f };
    bgfx::setUniform(m_uSunDir, sunDir);

    float sunCol[4] = { sky.sunColor.x, sky.sunColor.y, sky.sunColor.z, 0.0f };
    bgfx::setUniform(m_uSunColor, sunCol);

    float viewPos[4] = { camera.position.x, camera.position.y, camera.position.z, 0.0f };
    bgfx::setUniform(m_uViewPos, viewPos);

    float timeParam[4] = { (float)Time::TotalTime(), 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(m_uTime, timeParam);

    bgfx::setTexture(0, m_sSkybox, m_cubemapTexture);
    bgfx::setVertexBuffer(0, m_vbo);

    glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
    bgfx::setViewTransform(viewId, glm::value_ptr(view), glm::value_ptr(camera.GetProjectionMatrix()));

    uint64_t state = BGFX_STATE_WRITE_RGB 
                   | BGFX_STATE_WRITE_A 
                   | BGFX_STATE_DEPTH_TEST_LEQUAL 
                   | BGFX_STATE_CULL_CW;

    bgfx::setState(state);
    bgfx::submit(viewId, m_shader.GetProgram());
}

void R_Sky::Shutdown()
{
    if (bgfx::isValid(m_vbo)) 
        bgfx::destroy(m_vbo);
    if (bgfx::isValid(m_cubemapTexture)) 
        bgfx::destroy(m_cubemapTexture);
    if (bgfx::isValid(m_sSkybox))
    {
        bgfx::destroy(m_sSkybox);
        bgfx::destroy(m_uSkyParams);
        bgfx::destroy(m_uSunDir);
        bgfx::destroy(m_uSunColor);
        bgfx::destroy(m_uViewPos);
        bgfx::destroy(m_uTime);
    }
}