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
#include "cvar.h"
#include "timing.h"
#include "materials.h"
#include "r_sky.h"
#include "dynamic_sky.h"
#include "filesystem.h"
#include "console.h"
#include "dds.h"
#include <glm/gtc/type_ptr.hpp>

CVar r_sky_steps_primary("r_sky_steps_primary", "8", "Primary raymarching steps for dynamic sky.", CVAR_SAVE);
CVar r_sky_steps_light("r_sky_steps_light", "3", "Secondary light-sampling steps for dynamic sky.", CVAR_SAVE);

R_Sky::R_Sky() : m_vao(0), m_vbo(0), m_cubemapTexture(0) 
{
}

R_Sky::~R_Sky() 
{ 
    Shutdown(); 
}

bool R_Sky::Init(const std::string& skyName)
{
    if (m_cubemapTexture != 0) 
        glDeleteTextures(1, &m_cubemapTexture);

    m_shader.Load("shaders/sky.vert", "shaders/sky.frag");

    float skyboxVertices[] = 
    {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };

    glCreateVertexArrays(1, &m_vao);
    glCreateBuffers(1, &m_vbo);
    glNamedBufferData(m_vbo, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 3 * sizeof(float));
    glEnableVertexArrayAttrib(m_vao, 0);
    glVertexArrayAttribFormat(m_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vao, 0, 0);

    LoadCubemap(skyName);
    return true;
}

void R_Sky::LoadCubemap(const std::string& skyName)
{
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_cubemapTexture);
    glTextureParameteri(m_cubemapTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_cubemapTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_cubemapTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_cubemapTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_cubemapTexture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    std::vector<std::string> faces =
    {
        "right", "left", "top", "bottom", "front", "back"
    };

    bool firstAlloc = true;

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        std::string path = "textures/skybox/" + skyName + "_" + faces[i] + ".dds";

        int w, h, c;
        if (firstAlloc)
        {
            glTextureStorage2D(m_cubemapTexture, 10, GL_SRGB8_ALPHA8, 512, 512);
            firstAlloc = false;
        }

        if (!DDS::LoadCubemapFace(m_cubemapTexture, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, path, true))
        {
            Console::Warn("Skybox: face [" + faces[i] + "] missing - using fallback color.");
            uint8_t black[4] = { 0, 0, 0, 255 };
            glTextureSubImage3D(m_cubemapTexture, 0, 0, 0, i, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, black);
        }
    }
}

void R_Sky::Draw(const Camera& camera)
{
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    m_shader.Bind();
    
    // Remove translation from view matrix so sky stays at infinity
    glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix())); 
    m_shader.SetMat4("view", view);
    m_shader.SetMat4("projection", camera.GetProjectionMatrix());

    // Dynamic sky
    const auto& sky = DynamicSky::GetSettings();
    m_shader.SetInt("u_use_dynamic", sky.useDynamic ? 1 : 0);
    m_shader.SetInt("u_sky_steps_primary", r_sky_steps_primary.GetInt());
    m_shader.SetInt("u_sky_steps_light", r_sky_steps_light.GetInt());
    m_shader.SetVec3("u_sunDir", sky.sunDir);
    m_shader.SetVec3("u_sunColor", sky.sunColor);
    m_shader.SetVec3("u_cameraPos", camera.position);
    m_shader.SetFloat("u_time", (float)Time::TotalTime());

    glBindVertexArray(m_vao);
    glBindTextureUnit(0, m_cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void R_Sky::Shutdown()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteTextures(1, &m_cubemapTexture);
}