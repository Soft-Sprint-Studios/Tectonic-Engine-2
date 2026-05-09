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
#include <cstdio>
#include "pl_mpeg.h"
#include "r_video.h"
#include <r_state.h>
#include "video.h"
#include "filesystem.h"
#include "console.h"
#include <glm/gtc/matrix_transform.hpp>

R_VideoInstance::R_VideoInstance()
{
}

R_VideoInstance::~R_VideoInstance()
{
    Shutdown();
}

bool R_VideoInstance::Load(const std::string& path, bool loop)
{
    m_loop = loop;
    std::string fullPath = Filesystem::GetFullPath("videos/" + path);

    m_plm = plm_create_with_filename(fullPath.c_str());
    if (!m_plm)
    {
        Console::Error("Video: Failed to load " + path);
        return false;
    }

    plm_set_audio_enabled(m_plm, false);
    plm_set_loop(m_plm, m_loop);

    int w = plm_get_width(m_plm);
    int h = plm_get_height(m_plm);

    if (w == 0 || h == 0)
    {
        Console::Error("Video: " + path + " has 0 dimensions. Invalid MPEG");
        return false;
    }

    glGenTextures(1, &m_texY);
    glBindTexture(GL_TEXTURE_2D, m_texY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &m_texCb);
    glBindTexture(GL_TEXTURE_2D, m_texCb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &m_texCr);
    glBindTexture(GL_TEXTURE_2D, m_texCr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    return true;
}

void R_VideoInstance::Update(float dt)
{
    if (!m_plm)
    {
        return;
    }

    m_time += (double)dt;

    if (m_time >= m_nextFrameTime)
    {
        plm_frame_t* frame = plm_decode_video(m_plm);
        if (frame)
        {
            glBindTexture(GL_TEXTURE_2D, m_texY);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width, frame->height, GL_RED, GL_UNSIGNED_BYTE, frame->y.data);

            glBindTexture(GL_TEXTURE_2D, m_texCb);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width / 2, frame->height / 2, GL_RED, GL_UNSIGNED_BYTE, frame->cb.data);

            glBindTexture(GL_TEXTURE_2D, m_texCr);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width / 2, frame->height / 2, GL_RED, GL_UNSIGNED_BYTE, frame->cr.data);

            m_nextFrameTime += 1.0 / plm_get_framerate(m_plm);
        }
        else if (m_loop)
        {
            plm_rewind(m_plm);
            m_time = 0.0;
            m_nextFrameTime = 0.0;
        }
    }
}

void R_VideoInstance::BindTextures()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texY);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texCb);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_texCr);
}

void R_VideoInstance::Shutdown()
{
    if (m_plm)
    {
        plm_destroy(m_plm);
    }
    if (m_texY) 
        glDeleteTextures(1, &m_texY);
    if (m_texCb) 
        glDeleteTextures(1, &m_texCb);
    if (m_texCr) 
        glDeleteTextures(1, &m_texCr);
    m_plm = nullptr;
}

void R_Video::Init()
{
    m_shader.Load("shaders/video.vert", "shaders/video.frag");

    float verts[] =
    {
        -0.5f,  0.5f, 0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 0.0f
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void R_Video::Draw(const Camera& camera, const std::vector<std::shared_ptr<Video>>& videos)
{
    if (videos.empty())
    {
        return;
    }

    R_State::SetDepthTest(true);
    R_State::SetDepthMask(true);
    R_State::SetBlending(false);

    m_shader.Bind();
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_shader.SetInt("texY", 0);
    m_shader.SetInt("texCb", 1);
    m_shader.SetInt("texCr", 2);

    glBindVertexArray(m_vao);

    for (auto& v : videos)
    {
        if (!v->IsActive())
        {
            continue;
        }

        auto& def = v->GetDef();
        glm::mat4 model = glm::translate(glm::mat4(1.0f), def.position);
        model = glm::rotate(model, glm::radians(def.angles.y + 90.0f), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(-def.angles.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(def.angles.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, def.scale);

        m_shader.SetMat4("u_model", model);
        v->GetInternalPlayer()->BindTextures();

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
}

void R_Video::Shutdown()
{
    if (m_vao) 
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) 
        glDeleteBuffers(1, &m_vbo);
}