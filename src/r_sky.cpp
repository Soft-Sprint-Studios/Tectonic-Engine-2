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
#include "filesystem.h"
#include "console.h"
#include <stb_image.h>
#include <glm/gtc/type_ptr.hpp>

// Dynamic sky data
glm::vec3 R_Sky::s_sunDir = glm::vec3(0.0f, 1.0f, 0.01f);
bool R_Sky::s_useDynamic = false;

CVar r_sky_steps_primary("r_sky_steps_primary", "8", CVAR_SAVE);
CVar r_sky_steps_light("r_sky_steps_light", "3", CVAR_SAVE);

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

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    LoadCubemap(skyName);
    return true;
}

void R_Sky::LoadCubemap(const std::string& skyName)
{
    glGenTextures(1, &m_cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);

    std::vector<std::string> faces = 
    {
        "right", "left", "top", "bottom", "front", "back"
    };

    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        std::string path = Filesystem::GetFullPath("textures/skybox/" + skyName + "_" + faces[i] + ".png");

        int width, height, nrChannels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

        if (data)
        {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            GLenum internalFormat = (nrChannels == 4) ? GL_SRGB8_ALPHA8 : GL_SRGB8;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            Console::Warn("Skybox: face [" + faces[i] + "] missing - using fallback color.");
            uint8_t pink[3] = { 0, 0, 0 };
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pink);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void R_Sky::Draw(const Camera& camera)
{
    glDepthFunc(GL_LEQUAL);
    m_shader.Bind();
    
    // Remove translation from view matrix so sky stays at infinity
    glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix())); 
    m_shader.SetMat4("view", view);
    m_shader.SetMat4("projection", camera.GetProjectionMatrix());

    // Dynamic sky
    m_shader.SetInt("u_use_dynamic", s_useDynamic ? 1 : 0);
    m_shader.SetInt("u_sky_steps_primary", r_sky_steps_primary.GetInt());
    m_shader.SetInt("u_sky_steps_light", r_sky_steps_light.GetInt());
    m_shader.SetVec3("u_sunDir", s_sunDir);
    m_shader.SetVec3("u_cameraPos", camera.position);
    m_shader.SetFloat("u_time", (float)Time::TotalTime());

    glBindVertexArray(m_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
}

void R_Sky::Shutdown()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteTextures(1, &m_cubemapTexture);
}