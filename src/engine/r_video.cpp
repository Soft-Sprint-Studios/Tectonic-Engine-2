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
#include "r_bsp.h"
#include "entities.h"
#include "func_video.h"
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
        Console::Error("Video: " + path + " has 0 dimensions.");
        return false;
    }

    glGenTextures(1, &m_texY);
    glBindTexture(GL_TEXTURE_2D, m_texY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &m_texCb);
    glBindTexture(GL_TEXTURE_2D, m_texCb);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &m_texCr);
    glBindTexture(GL_TEXTURE_2D, m_texCr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, m_texY);
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_2D, m_texCb);
    glActiveTexture(GL_TEXTURE12);
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
}

void R_Video::Draw(const Camera& camera, R_BSP* bsp)
{
    R_State::SetDepthTest(true);
    R_State::SetDepthMask(true);
    R_State::SetBlending(false);

    m_shader.Bind();
    m_shader.SetMat4("u_view", camera.GetViewMatrix());
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());

    m_shader.SetInt("texY", 10);
    m_shader.SetInt("texCb", 11);
    m_shader.SetInt("texCr", 12);

    for (const auto& ent : EntityManager::GetEntities())
    {
        if (ent->GetClassName() != "func_video" || !ent->IsEnabled())
        {
            continue;
        }

        auto video_ent = std::dynamic_pointer_cast<FuncVideo>(ent);
        if (!video_ent)
        {
            continue;
        }

        auto video_handle = video_ent->GetHandle();
        if (!video_handle || !video_handle->IsActive())
        {
            continue;
        }

        glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
        glm::vec3 ang = ent->GetAngles();
        model = glm::rotate(model, glm::radians(ang.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(ang.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(ang.z), glm::vec3(0, 0, 1));

        m_shader.SetMat4("u_model", model);
        video_handle->GetInternalPlayer()->BindTextures();

        bsp->DrawBModel(ent->GetBModelIndex(), m_shader, model, true);
    }
}

void R_Video::Shutdown()
{
}