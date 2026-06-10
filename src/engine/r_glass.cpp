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
#include <resources.h>
#include "r_glass.h"
#include "camera.h"
#include "renderer.h"
#include "entities.h"
#include "r_bsp.h"

void R_Glass::Init(int width, int height)
{
    m_shader.Load("shaders/glass.vert", "shaders/glass.frag");

    glGenTextures(1, &m_refractTex);
    Rescale(width, height);
}

void R_Glass::Rescale(int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, m_refractTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (m_fbo == 0)
    {
        glGenFramebuffers(1, &m_fbo);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_refractTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_Glass::CaptureScreen(GLuint screenFBO, int width, int height)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, screenFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
}

void R_Glass::Draw(const Camera& camera, R_BSP* bsp)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    m_shader.Bind();
    m_shader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_shader.SetMat4("u_view", camera.GetViewMatrix());

    glBindTextureUnit(10, m_refractTex);

    for (auto& ent : EntityManager::GetEntities())
    {
        if (ent->GetClassName() == "func_glass" && ent->IsEnabled())
        {
            std::string normName = ent->GetValue("normal", "water_normal.dds");
            auto normTex = Resources::LoadTexture("textures/" + normName, false);
            if (normTex)
            {
                normTex->Bind(0);
            }

            glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
            m_shader.SetMat4("u_model", model);
            m_shader.SetFloat("u_amount", ent->GetFloat("refractamount", 0.1f));

            bsp->DrawBModel(ent->GetBModelIndex(), m_shader, model, true);
        }
    }

    glDepthMask(GL_TRUE);
}

void R_Glass::Shutdown()
{
    if (m_fbo) 
        glDeleteFramebuffers(1, &m_fbo);
    if (m_refractTex) 
        glDeleteTextures(1, &m_refractTex);
}