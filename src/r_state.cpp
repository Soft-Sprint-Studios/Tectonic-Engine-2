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
#include "r_state.h"

namespace R_State
{
    static glm::vec4 s_clearColor = {0,0,0,0};
    static glm::bvec4 s_colorMask = { true, true, true, true };
    static bool s_depthEnabled = false;
    static bool s_depthMask = true;
    static GLenum s_depthFunc = GL_LESS;
    static bool s_blendEnabled = false;
    static bool s_cullEnabled = false;
    static bool s_msEnabled = false;

    void SetClearColor(const glm::vec4& color)
    {
        if (color != s_clearColor)
        {
            glClearColor(color.r, color.g, color.b, color.a);
            s_clearColor = color;
        }
    }

    void SetColorMask(bool r, bool g, bool b, bool a)
    {
        glm::bvec4 newMask(r, g, b, a);

        if (newMask != s_colorMask)
        {
            glColorMask(r ? GL_TRUE : GL_FALSE, g ? GL_TRUE : GL_FALSE, b ? GL_TRUE : GL_FALSE, a ? GL_TRUE : GL_FALSE);

            s_colorMask = newMask;
        }
    }

    void Clear(bool color, bool depth, bool stencil)
    {
        GLbitfield mask = 0;

        if (color) 
        {
            mask |= GL_COLOR_BUFFER_BIT;
        }

        if (depth) 
        {
            mask |= GL_DEPTH_BUFFER_BIT;
        }

        if (stencil) 
        {
            mask |= GL_STENCIL_BUFFER_BIT;
        }

        glClear(mask);
    }

    void SetViewport(int x, int y, int w, int h)
    {
        glViewport(x, y, w, h);
    }

    void SetDepthTest(bool enable)
    {
        if (enable != s_depthEnabled)
        {
            enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
            s_depthEnabled = enable;
        }
    }

    void SetDepthMask(bool enable)
    {
        if (enable != s_depthMask)
        {
            glDepthMask(enable ? GL_TRUE : GL_FALSE);
            s_depthMask = enable;
        }
    }

    void SetDepthFunc(GLenum func)
    {
        if (func != s_depthFunc)
        {
            glDepthFunc(func);
            s_depthFunc = func;
        }
    }

    void SetBlending(bool enable)
    {
        if (enable != s_blendEnabled)
        {
            enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
            s_blendEnabled = enable;
        }
    }

    void SetBlendFunc(GLenum src, GLenum dst)
    {
        glBlendFunc(src, dst);
    }

    void SetCulling(bool enable)
    {
        if (enable != s_cullEnabled)
        {
            enable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
            s_cullEnabled = enable;
        }
    }

    void SetCullFace(GLenum face)
    {
        glCullFace(face);
    }

    void SetWireframe(bool enable)
    {
        glPolygonMode(GL_FRONT_AND_BACK, enable ? GL_LINE : GL_FILL);
    }

    void SetMultisample(bool enable)
    {
        if (enable != s_msEnabled)
        {
            enable ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);
            s_msEnabled = enable;
        }
    }
}