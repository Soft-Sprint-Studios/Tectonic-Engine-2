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
#include "r_sprites.h"
#include "materials.h"
#include <glm/gtc/type_ptr.hpp>

void R_Sprites::Init()
{
    m_shader.Load("shaders/sprite.vert", "shaders/sprite.frag");

    m_layout.begin()
        .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    float quad[] = 
    {
        -0.5f,  0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f, 1.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 1.0f
    };

    m_vbo = bgfx::createVertexBuffer(bgfx::copy(quad, sizeof(quad)), m_layout);

    m_sTexture = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
    m_uColor = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
    m_uSpriteParams = bgfx::createUniform("u_spriteParams", bgfx::UniformType::Vec4);
    m_uViewRightLocal = bgfx::createUniform("u_viewRightLocal", bgfx::UniformType::Vec4);
    m_uViewUpLocal = bgfx::createUniform("u_viewUpLocal", bgfx::UniformType::Vec4);
    m_uWorldPosLocal = bgfx::createUniform("u_worldPosLocal", bgfx::UniformType::Vec4);
}

void R_Sprites::Draw(bgfx::ViewId viewId, const Camera& camera, const std::vector<std::shared_ptr<Sprite>>& sprites)
{
    if (sprites.empty()) 
        return;

    uint64_t state = BGFX_STATE_WRITE_RGB 
                   | BGFX_STATE_WRITE_A 
                   | BGFX_STATE_DEPTH_TEST_LESS 
                   | BGFX_STATE_BLEND_ALPHA;

    glm::mat4 v = camera.GetViewMatrix();
    float viewRight[4] = { v[0][0], v[1][0], v[2][0], 0.0f };
    float viewUp[4] = { v[0][1], v[1][1], v[2][1], 0.0f };

    bgfx::setUniform(m_uViewRightLocal, viewRight);
    bgfx::setUniform(m_uViewUpLocal, viewUp);

    glm::mat4 identity(1.0f);

    for (const auto& sprite : sprites)
    {
        if (!sprite->IsActive()) 
            continue;

        const auto& def = sprite->GetDef();
        auto tex = Materials::GetTexture(def.textureName);
        if (tex)
        {
            bgfx::setTransform(glm::value_ptr(identity));

            bgfx::setTexture(0, m_sTexture, tex->GetHandle());

            float worldPos[4] = { sprite->GetPosition().x, sprite->GetPosition().y, sprite->GetPosition().z, 0.0f };
            bgfx::setUniform(m_uWorldPosLocal, worldPos);

            float spriteParams[4] = { def.scale.x, def.scale.y, def.cylindrical ? 1.0f : 0.0f, 0.0f };
            bgfx::setUniform(m_uSpriteParams, spriteParams);
            bgfx::setUniform(m_uColor, glm::value_ptr(def.color));

            bgfx::setVertexBuffer(0, m_vbo);
            bgfx::setState(state);
            bgfx::submit(viewId, m_shader.GetProgram());
        }
    }
}

void R_Sprites::Shutdown()
{
    if (bgfx::isValid(m_vbo)) 
        bgfx::destroy(m_vbo);
    if (bgfx::isValid(m_sTexture))
    {
        bgfx::destroy(m_sTexture);
        bgfx::destroy(m_uColor);
        bgfx::destroy(m_uSpriteParams);
        bgfx::destroy(m_uViewRightLocal);
        bgfx::destroy(m_uViewUpLocal);
        bgfx::destroy(m_uWorldPosLocal);
    }
    m_vbo = BGFX_INVALID_HANDLE;
    m_sTexture = BGFX_INVALID_HANDLE;
}