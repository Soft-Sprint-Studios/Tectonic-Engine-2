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
#include "bsploader.h"
#include "r_shader.h"
#include "r_texture.h"
#include <memory>
#include <vector>

struct BSPDrawCall
{
    std::shared_ptr<R_Texture> texture;
    std::shared_ptr<R_Texture> normalMap;
    std::shared_ptr<R_Texture> specularMap;
    bool isBumped;

    uint32_t start;
    uint32_t count;
    glm::vec3 mins;
    glm::vec3 maxs;
};

class R_BSP
{
public:
    R_BSP();
    ~R_BSP();

    bool Init(const BSP::MapData& mapData);
    void Draw(const R_Shader& shader, const Frustum& frustum, bool depthOnly = false);
    void Shutdown();

    GLuint GetVAO() const 
    { 
        return m_vao; 
    }

private:
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_lightmapTexture = 0;
    std::vector<BSPDrawCall> m_drawCalls;
};