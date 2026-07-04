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
#include "r_cubemap.h"
#include "cubemap.h"
#include "renderer.h"
#include "camera.h"
#include "entities.h"
#include "filesystem.h"
#include "dds.h"
#include "cvar.h"
#include "physics.h"
#include <bgfx/bgfx.h>
#include <vector>
#include <fstream>

namespace R_Cubemap
{
    CVar r_cubemap_res("r_cubemap_resolution", "512", "Resolution of rendered cubemap faces.", CVAR_SAVE);

    uint32_t CreateFromFiles(const std::string& basePath)
    {
        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
        const char* faces[] = { "_right", "_left", "_top", "_bottom", "_front", "_back" };
        
        for (int i = 0; i < 6; i++)
        {
            DDS::ImageInfo info;
            if (DDS::Load(basePath + faces[i] + ".dds", true, info) && !info.mips.empty())
            {
                if (!bgfx::isValid(handle))
                {
                    bool hasMips = info.mips.size() > 1;
                    uint64_t flags = BGFX_TEXTURE_SRGB | BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP;
                    handle = bgfx::createTextureCube((uint16_t)info.width, hasMips, 1, bgfx::TextureFormat::RGBA8, flags);
                }

                for (size_t m = 0; m < info.mips.size(); ++m)
                {
                    const bgfx::Memory* mem = bgfx::copy(&info.data[info.mips[m].offset], (uint32_t)info.mips[m].size);
                    bgfx::updateTextureCube(handle, 0, (uint8_t)i, (uint8_t)m, 0, 0, (uint16_t)info.mips[m].width, (uint16_t)info.mips[m].height, mem);
                }
            }
        }

        return handle.idx;
    }

    void BuildProbes(const std::string& mapName, Renderer* renderer)
    {
        auto ents = EntityManager::GetEntities();
        std::vector<std::shared_ptr<Entity>> probes;
        for (auto& e : ents)
        {
            if (e->GetClassName() == "env_cubemap_box")
            {
                probes.push_back(e);
            }
        }

        if (probes.empty())
        {
            return;
        }

        int res = r_cubemap_res.GetInt();

        uint64_t rtFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        bgfx::TextureHandle rtColor = bgfx::createTexture2D((uint16_t)res, (uint16_t)res, false, 1, bgfx::TextureFormat::RGBA8, rtFlags);
        bgfx::TextureHandle rtDepth = bgfx::createTexture2D((uint16_t)res, (uint16_t)res, false, 1, bgfx::TextureFormat::D24S8, rtFlags);
        
        bgfx::TextureHandle attachments[] = { rtColor, rtDepth };
        bgfx::FrameBufferHandle fbo = bgfx::createFrameBuffer(2, attachments, true);
        
        bgfx::TextureHandle readbackTex = bgfx::createTexture2D((uint16_t)res, (uint16_t)res, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);

        std::string dir = "cubemaps/" + mapName + "/";
        Filesystem::CreateDirectory(dir);

        Camera cam(90.0f, 1.0f, 0.1f, 2000.0f);
        const char* faceNames[] = { "right", "left", "top", "bottom", "front", "back" };

        for (size_t i = 0; i < probes.size(); i++)
        {
            btVector3 bmin, bmax;
            probes[i]->GetPhysObject()->getCollisionShape()->getAabb(probes[i]->GetPhysObject()->getWorldTransform(), bmin, bmax);
            glm::vec3 origin = (glm::vec3(bmin.x(), bmin.y(), bmin.z()) + glm::vec3(bmax.x(), bmax.y(), bmax.z())) * 0.5f;
            cam.position = origin;

            for (int j = 0; j < 6; j++)
            {
                cam.yaw = (j == 0) ? 0.0f : (j == 1) ? 180.0f : (j == 4) ? 90.0f : (j == 5) ? -90.0f : -90.0f;
                cam.pitch = (j == 2) ? 90.0f : (j == 3) ? -90.0f : 0.0f;

                renderer->RenderWorld(cam, 0, false, RenderView::GBuffer, RenderView::Resolve, fbo);

                bgfx::blit(RenderView::Resolve + 1, readbackTex, 0, 0, rtColor);
                
                std::vector<uint8_t> buffer(res * res * 4);

                uint32_t targetFrame = bgfx::readTexture(readbackTex, buffer.data());

                uint32_t currentFrame = bgfx::frame();
                while (currentFrame <= targetFrame)
                {
                    currentFrame = bgfx::frame();
                }

                DDS::WriteUncompressedRGB(Filesystem::GetFullPath(dir + "cubemap_" + std::to_string(i) + "_" + faceNames[j] + ".dds"), res, res, buffer.data());
            }

            std::ofstream meta(Filesystem::GetFullPath(dir + "cubemap_" + std::to_string(i) + ".txt"));
            meta << origin.x << " " << origin.y << " " << origin.z << "\n";
            meta << bmin.x() << " " << bmin.y() << " " << bmin.z() << "\n";
            meta << bmax.x() << " " << bmax.y() << " " << bmax.z() << "\n";
        }

        bgfx::destroy(fbo); 
        bgfx::destroy(readbackTex);

        Cubemap::LoadForMap(mapName);
    }

    void Release(uint32_t id)
    {
        if (id != 0 && id != bgfx::kInvalidHandle)
        {
            bgfx::TextureHandle handle = { (uint16_t)id };
            if (bgfx::isValid(handle))
            {
                bgfx::destroy(handle);
            }
        }
    }
}