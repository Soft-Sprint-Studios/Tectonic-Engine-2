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
#include "r_motionblur.h"
#include "cvar.h"

CVar r_motionblur("r_motionblur", "1", "Enable Motion Blur.", CVAR_SAVE);
CVar r_motionblur_samples("r_motionblur_samples", "16", "Number of samples for motion blur.", CVAR_SAVE);
CVar r_motionblur_scale("r_motionblur_scale", "1.0", "Intensity of the motion blur effect.", CVAR_SAVE);

R_MotionBlur::R_MotionBlur()
{
}

R_MotionBlur::~R_MotionBlur()
{
    Shutdown();
}

bool R_MotionBlur::Init(int width, int height)
{
    m_shader.Load("shaders/motion_blur.vert", "shaders/motion_blur.frag");
    CreateBuffers(width, height);
    return true;
}

void R_MotionBlur::CreateBuffers(int width, int height)
{
    m_width = width;
    m_height = height;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_MotionBlur::Render(GLuint sceneTex, GLuint velocityTex, GLuint quadVAO)
{
    if (r_motionblur.GetInt() == 0)
    {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);

    m_shader.Bind();
    m_shader.SetInt("u_samples", r_motionblur_samples.GetInt());
    m_shader.SetFloat("u_blurScale", r_motionblur_scale.GetFloat());

    glBindTextureUnit(0, sceneTex);
    glBindTextureUnit(1, velocityTex);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_MotionBlur::Bind(const R_Shader& shader)
{
    if (r_motionblur.GetInt() > 0)
    {
        shader.SetInt("u_motionblur_enabled", 1);
        glBindTextureUnit(7, m_texture);
    }
    else
    {
        shader.SetInt("u_motionblur_enabled", 0);
    }
}

void R_MotionBlur::DeleteBuffers()
{
    if (m_fbo)
    {
        glDeleteFramebuffers(1, &m_fbo);
    }
    if (m_texture)
    {
        glDeleteTextures(1, &m_texture);
    }
}

void R_MotionBlur::Rescale(int w, int h)
{
    DeleteBuffers();
    CreateBuffers(w, h);
}

void R_MotionBlur::Shutdown()
{
    DeleteBuffers();
}