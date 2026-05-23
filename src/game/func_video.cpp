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
#include "func_video.h"
#include "video.h"

void FuncVideo::Spawn(const BSP::EntityData& entData)
{
    Entity::Spawn(entData);

    VideoDef def;
    def.videoPath = GetValue("video");
    def.loop = GetInt("loop", 1) != 0;

    m_handle = Videos::CreateVideo(def);
}

void FuncVideo::SetEnabled(bool state)
{
    Entity::SetEnabled(state);
    if (m_handle)
    {
        m_handle->SetActive(state);
    }
}

// Must be false because we render in r_video
bool FuncVideo::IsRenderable() const
{
    return false;
}

bool FuncVideo::IsCollidable() const
{
    return true;
}

LINK_ENTITY_TO_CLASS("func_video", FuncVideo)