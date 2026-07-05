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
#include "r_video.h"
#include "video.h"
#include "filesystem.h"
#include "console.h"
#include "r_bsp.h"
#include "entities.h"
#include "func_video.h"
#include "timing.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

    m_texY = bgfx::createTexture2D((uint16_t)w, (uint16_t)h, false, 1, bgfx::TextureFormat::R8, BGFX_TEXTURE_NONE);
    m_texCb = bgfx::createTexture2D((uint16_t)(w / 2), (uint16_t)(h / 2), false, 1, bgfx::TextureFormat::R8, BGFX_TEXTURE_NONE);
    m_texCr = bgfx::createTexture2D((uint16_t)(w / 2), (uint16_t)(h / 2), false, 1, bgfx::TextureFormat::R8, BGFX_TEXTURE_NONE);

    return true;
}

void R_VideoInstance::Update(float dt)
{
    if (!m_plm) 
        return;

    m_time += (double)dt;

    if (m_time >= m_nextFrameTime)
    {
        plm_frame_t* frame = plm_decode_video(m_plm);
        if (frame)
        {
            const bgfx::Memory* memY = bgfx::copy(frame->y.data, frame->width * frame->height);
            bgfx::updateTexture2D(m_texY, 0, 0, 0, 0, (uint16_t)frame->width, (uint16_t)frame->height, memY);

            const bgfx::Memory* memCb = bgfx::copy(frame->cb.data, (frame->width / 2) * (frame->height / 2));
            bgfx::updateTexture2D(m_texCb, 0, 0, 0, 0, (uint16_t)(frame->width / 2), (uint16_t)(frame->height / 2), memCb);

            const bgfx::Memory* memCr = bgfx::copy(frame->cr.data, (frame->width / 2) * (frame->height / 2));
            bgfx::updateTexture2D(m_texCr, 0, 0, 0, 0, (uint16_t)(frame->width / 2), (uint16_t)(frame->height / 2), memCr);

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

void R_VideoInstance::BindTextures(bgfx::UniformHandle s_texY, bgfx::UniformHandle s_texCb, bgfx::UniformHandle s_texCr)
{
    bgfx::setTexture(10, s_texY, m_texY);
    bgfx::setTexture(11, s_texCb, m_texCb);
    bgfx::setTexture(12, s_texCr, m_texCr);
}

void R_VideoInstance::Shutdown()
{
    if (m_plm)
    {
        plm_destroy(m_plm);
        m_plm = nullptr;
    }
    if (bgfx::isValid(m_texY))
        bgfx::destroy(m_texY);
    if (bgfx::isValid(m_texCb)) 
        bgfx::destroy(m_texCb);
    if (bgfx::isValid(m_texCr)) 
        bgfx::destroy(m_texCr);
    m_texY = m_texCb = m_texCr = BGFX_INVALID_HANDLE;
}

void R_Video::Init()
{
    m_shader.Load("shaders/video.vert", "shaders/video.frag");
    m_sTexY = bgfx::createUniform("texY", bgfx::UniformType::Sampler);
    m_sTexCb = bgfx::createUniform("texCb", bgfx::UniformType::Sampler);
    m_sTexCr = bgfx::createUniform("texCr", bgfx::UniformType::Sampler);
}

void R_Video::Draw(bgfx::ViewId viewId, const Camera& camera, R_BSP* bsp)
{
    for (const auto& ent : EntityManager::GetEntities())
    {
        if (ent->GetClassName() != "func_video" || !ent->IsEnabled())
            continue;

        auto video_ent = std::dynamic_pointer_cast<FuncVideo>(ent);
        if (!video_ent) 
            continue;

        auto video_handle = video_ent->GetHandle();
        if (!video_handle || !video_handle->IsActive()) 
            continue;

        auto it = m_instances.find(video_handle.get());
        if (it == m_instances.end())
        {
            auto instance = std::make_unique<R_VideoInstance>();
            instance->Load(video_handle->GetDef().videoPath, video_handle->GetDef().loop);
            m_instances[video_handle.get()] = std::move(instance);
            it = m_instances.find(video_handle.get());
        }

        it->second->Update(Time::DeltaTime());

        glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
        glm::vec3 ang = ent->GetAngles();
        model = glm::rotate(model, glm::radians(ang.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(ang.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(ang.z), glm::vec3(0, 0, 1));

        it->second->BindTextures(m_sTexY, m_sTexCb, m_sTexCr);

        bsp->DrawBModel(ent->GetBModelIndex(), m_shader, viewId, model, camera.position, true);
    }
}

void R_Video::Shutdown()
{
    m_instances.clear();
    if (bgfx::isValid(m_sTexY))
    {
        bgfx::destroy(m_sTexY);
        bgfx::destroy(m_sTexCb);
        bgfx::destroy(m_sTexCr);
        m_sTexY = m_sTexCb = m_sTexCr = BGFX_INVALID_HANDLE;
    }
}