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
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace R_State
{
    void SetClearColor(const glm::vec4& color);
    void SetColorMask(bool r, bool g, bool b, bool a);
    void Clear(bool color, bool depth, bool stencil);
    
    void SetViewport(int x, int y, int w, int h);
    
    void SetDepthTest(bool enable);
    void SetDepthMask(bool enable);
    void SetDepthFunc(GLenum func);
    
    void SetBlending(bool enable);
    void SetBlendFunc(GLenum src, GLenum dst);
    
    void SetCulling(bool enable);
    void SetCullFace(GLenum face);
    
    void SetWireframe(bool enable);

    void SetMultisample(bool enable);
}