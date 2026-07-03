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
#include "renderer.h"
#include "r_ui.h"
#include "filesystem.h"
#include "console.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

R_UI::R_UI()
{
}

R_UI::~R_UI()
{
    Shutdown();
}

void R_UI::Init(Window* window)
{
    if (!TTF_Init())
    {
        Console::Error("Failed to initialize SDL_ttf: " + std::string(SDL_GetError()));
        return;
    }

    std::string fontPath = Filesystem::GetFullPath("fonts/LiberationSans-Regular.ttf");
    m_font = TTF_OpenFont(fontPath.c_str(), 18);
    if (!m_font)
    {
        Console::Error("Failed to load font: " + fontPath);
        return;
    }

    m_uiLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    uint8_t whitePixel[] = { 255, 255, 255, 255 };
    m_whiteTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy(whitePixel, 4));

    m_uTextColor = bgfx::createUniform("u_textColor", bgfx::UniformType::Vec4);
    m_uUiParams = bgfx::createUniform("u_uiParams", bgfx::UniformType::Vec4);
    m_sTexture = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);

    m_shader.Load("shaders/ui.vert", "shaders/ui.frag");

    int w, h;
    SDL_GetWindowSize(window->Get(), &w, &h);
    OnWindowResize(w, h);

    m_viewId = RenderView::UI;
}

void R_UI::Shutdown()
{
    for (auto const& [text, cached] : m_textCache)
    {
        if (bgfx::isValid(cached.texture))
        {
            bgfx::destroy(cached.texture);
        }
    }
    m_textCache.clear();
    m_commands.clear();
    m_rectCommands.clear();

    if (m_font)
    {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }

    if (bgfx::isValid(m_whiteTexture))
    {
        bgfx::destroy(m_whiteTexture);
        m_whiteTexture = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_uTextColor))
    {
        bgfx::destroy(m_uTextColor);
        bgfx::destroy(m_uUiParams);
        bgfx::destroy(m_sTexture);
        m_uTextColor = BGFX_INVALID_HANDLE;
    }

    if (TTF_WasInit())
    {
        TTF_Quit();
    }
}

void R_UI::OnWindowResize(int w, int h)
{
    m_projection = glm::ortho(0.0f, (float)w, (float)h, 0.0f, -1.0f, 1.0f);
    bgfx::setViewRect(m_viewId, 0, 0, (uint16_t)w, (uint16_t)h);
    bgfx::setViewTransform(m_viewId, nullptr, glm::value_ptr(m_projection));
}

void R_UI::DrawText(const std::string& text, float x, float y, const glm::vec4& color, float scale)
{
    m_commands.push_back({ text, x, y, color, scale });
}

void R_UI::DrawRect(float x, float y, float w, float h, const glm::vec4& color)
{
    m_rectCommands.push_back({ x, y, w, h, color, 0 });
}

void R_UI::DrawCircle(float x, float y, float w, float h, const glm::vec4& color)
{
    m_rectCommands.push_back({ x, y, w, h, color, 1 });
}

bool R_UI::IsMouseOver(float x, float y, float w, float h) const
{
    float mx, my;
    SDL_GetMouseState(&mx, &my);
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
}

void R_UI::Render()
{
    if (!m_font || (m_commands.empty() && m_rectCommands.empty()))
    {
        m_commands.clear();
        m_rectCommands.clear();
        return;
    }

    uint64_t state = BGFX_STATE_WRITE_RGB
        | BGFX_STATE_WRITE_A
        | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

    // Draw Rectangles and Circles first
    if (!m_rectCommands.empty() && bgfx::isValid(m_whiteTexture))
    {
        for (const auto& rect : m_rectCommands)
        {
            bgfx::TransientVertexBuffer tvb;
            bgfx::allocTransientVertexBuffer(&tvb, 6, m_uiLayout);

            UIVertex* v = (UIVertex*)tvb.data;
            v[0] = { rect.x,          rect.y + rect.h, 0.0f, 0.0f, 1.0f };
            v[1] = { rect.x,          rect.y,          0.0f, 0.0f, 0.0f };
            v[2] = { rect.x + rect.w, rect.y,          0.0f, 1.0f, 0.0f };
            v[3] = { rect.x,          rect.y + rect.h, 0.0f, 0.0f, 1.0f };
            v[4] = { rect.x + rect.w, rect.y,          0.0f, 1.0f, 0.0f };
            v[5] = { rect.x + rect.w, rect.y + rect.h, 0.0f, 1.0f, 1.0f };

            float params[4] = { (float)rect.type, 0.0f, 0.0f, 0.0f };
            bgfx::setUniform(m_uUiParams, params);
            bgfx::setUniform(m_uTextColor, glm::value_ptr(rect.color));

            bgfx::setTexture(0, m_sTexture, m_whiteTexture);
            bgfx::setVertexBuffer(0, &tvb);
            bgfx::setState(state);
            bgfx::submit(m_viewId, m_shader.GetProgram());
        }
        m_rectCommands.clear();
    }

    // Draw Text
    for (const auto& cmd : m_commands)
    {
        if (m_textCache.find(cmd.text) == m_textCache.end())
        {
            SDL_Color sdlColor = { 255, 255, 255, 255 };
            SDL_Surface* surface = TTF_RenderText_Blended(m_font, cmd.text.c_str(), 0, sdlColor);
            if (!surface)
            {
                continue;
            }

            SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(surface);
            if (!converted)
            {
                continue;
            }

            CachedText newCache;
            newCache.width = converted->w;
            newCache.height = converted->h;

            const bgfx::Memory* mem = bgfx::copy(converted->pixels, converted->w * converted->h * 4);
            newCache.texture = bgfx::createTexture2D(
                (uint16_t)converted->w,
                (uint16_t)converted->h,
                false, 1,
                bgfx::TextureFormat::RGBA8,
                BGFX_TEXTURE_NONE,
                mem
            );

            m_textCache[cmd.text] = newCache;
            SDL_DestroySurface(converted);
        }

        CachedText& cached = m_textCache[cmd.text];
        if (!bgfx::isValid(cached.texture))
        {
            continue;
        }

        float xpos = cmd.x;
        float ypos = cmd.y;
        float w = (float)cached.width * cmd.scale;
        float h = (float)cached.height * cmd.scale;

        bgfx::TransientVertexBuffer tvb;
        bgfx::allocTransientVertexBuffer(&tvb, 6, m_uiLayout);

        UIVertex* v = (UIVertex*)tvb.data;
        v[0] = { xpos,     ypos + h, 0.0f, 0.0f, 1.0f };
        v[1] = { xpos,     ypos,     0.0f, 0.0f, 0.0f };
        v[2] = { xpos + w, ypos,     0.0f, 1.0f, 0.0f };
        v[3] = { xpos,     ypos + h, 0.0f, 0.0f, 1.0f };
        v[4] = { xpos + w, ypos,     0.0f, 1.0f, 0.0f };
        v[5] = { xpos + w, ypos + h, 0.0f, 1.0f, 1.0f };

        float params[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(m_uUiParams, params);
        bgfx::setUniform(m_uTextColor, glm::value_ptr(cmd.color));

        bgfx::setTexture(0, m_sTexture, cached.texture);
        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setState(state);
        bgfx::submit(m_viewId, m_shader.GetProgram());
    }

    m_commands.clear();
}