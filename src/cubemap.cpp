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
#include "cubemap.h"
#include "console.h"
#include "filesystem.h"
#include "entities.h"
#include "renderer.h"
#include "camera.h"
#include "maps.h"
#include "cvar.h"
#include <vector>
#include <stb_image.h>
#include <glm/gtx/norm.hpp>
#include <fstream>
#include <sstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>


namespace Cubemap
{
    static std::vector<CubemapProbe> s_probes;
    CVar r_cubemap_resolution("r_cubemap_resolution", "512", CVAR_SAVE);

    void Init() 
    {
    }

    void Shutdown() 
    { 
        Clear(); 
    }

    void Clear()
    {
        for (auto& probe : s_probes)
        {
            if (probe.textureID != 0)
            {
                glDeleteTextures(1, &probe.textureID);
            }
        }
        s_probes.clear();
    }

    const std::vector<CubemapProbe>& GetProbes()
    {
        return s_probes;
    }

    const CubemapProbe* FindClosest(const glm::vec3& position)
    {
        if (s_probes.empty())
        {
            return nullptr;
        }

        const CubemapProbe* closest = &s_probes[0];
        float min_dist_sq = glm::length2(position - closest->origin);

        for (size_t i = 1; i < s_probes.size(); ++i)
        {
            float dist_sq = glm::length2(position - s_probes[i].origin);
            if (dist_sq < min_dist_sq)
            {
                min_dist_sq = dist_sq;
                closest = &s_probes[i];
            }
        }
        return closest;
    }

    void LoadForMap(const std::string& mapName)
    {
        Clear();
        std::string cubemapDir = "cubemaps/" + mapName + "/";
        if (!Filesystem::Exists(cubemapDir))
        {
            Console::Warn("No cubemaps found for map: " + mapName);
            return;
        }

        int probeIndex = 0;
        while (true)
        {
            std::string basePath = cubemapDir + "cubemap_" + std::to_string(probeIndex);
            std::string metaPath = basePath + ".txt";

            if (!Filesystem::Exists(metaPath))
            {
                break;
            }

            CubemapProbe probe;

            std::string metaContent = Filesystem::ReadText(metaPath);
            std::stringstream ss(metaContent);
            ss >> probe.origin.x >> probe.origin.y >> probe.origin.z;
            ss >> probe.mins.x >> probe.mins.y >> probe.mins.z;
            ss >> probe.maxs.x >> probe.maxs.y >> probe.maxs.z;

            glGenTextures(1, &probe.textureID);
            glBindTexture(GL_TEXTURE_CUBE_MAP, probe.textureID);

            std::vector<std::string> faces = { "right", "left", "top", "bottom", "front", "back" };
            stbi_set_flip_vertically_on_load(false);

            for (unsigned int i = 0; i < faces.size(); i++)
            {
                std::string facePath = Filesystem::GetFullPath(basePath + "_" + faces[i] + ".png");
                int width, height, nrChannels;
                unsigned char* data = stbi_load(facePath.c_str(), &width, &height, &nrChannels, 0);
                if (data)
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                    stbi_image_free(data);
                }
                else
                {
                    Console::Error("Failed to load cubemap face: " + facePath);
                }
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

            s_probes.push_back(probe);
            probeIndex++;
        }
    }

    void BuildCubemaps(const std::string& mapName, Renderer* renderer)
    {
        Console::Log("Building cubemaps for " + mapName + "...");

        std::vector<std::shared_ptr<Entity>> cubemapEntities;
        for (const auto& ent : EntityManager::GetEntities())
        {
            if (ent->GetClassName() == "env_cubemap_box")
            {
                cubemapEntities.push_back(ent);
            }
        }

        if (cubemapEntities.empty())
        {
            return;
        }
        
        const int resolution = r_cubemap_resolution.GetInt();

        GLuint fbo, rbo;
        glGenFramebuffers(1, &fbo);
        glGenRenderbuffers(1, &rbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

        Camera buildCam(90.0f, 1.0f, 0.1f, 2000.0f);

        glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 2000.0f);
        glm::mat4 views[] =
        {
           glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // right
           glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // left
           glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // top
           glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // bottom
           glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // front
           glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))  // back
        };

        std::string cubemapDir = "cubemaps/" + mapName + "/";
        Filesystem::CreateDirectory("cubemaps/");
        Filesystem::CreateDirectory(cubemapDir);

        for (size_t i = 0; i < cubemapEntities.size(); ++i)
        {
            auto& ent = cubemapEntities[i];
            
            btVector3 aabbMin, aabbMax;
            ent->GetPhysObject()->getCollisionShape()->getAabb(ent->GetPhysObject()->getWorldTransform(), aabbMin, aabbMax);
            
            glm::vec3 mins = { aabbMin.x(), aabbMin.y(), aabbMin.z() };
            glm::vec3 maxs = { aabbMax.x(), aabbMax.y(), aabbMax.z() };
            glm::vec3 origin = (mins + maxs) * 0.5f;

            GLuint cubemapTex;
            glGenTextures(1, &cubemapTex);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
            for (int j = 0; j < 6; ++j)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_RGB, resolution, resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glViewport(0, 0, resolution, resolution);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            buildCam.position = origin;

            for (int j = 0; j < 6; ++j)
            {
                buildCam.yaw = 0; 
                buildCam.pitch = 0;

                if(j==0) 
                    buildCam.yaw = 0;
                if(j==1) 
                    buildCam.yaw = 180;
                if(j==2) 
                    buildCam.pitch = 90;
                if(j==3) 
                    buildCam.pitch = -90;
                if(j==4) 
                    buildCam.yaw = 90;
                if(j==5) 
                    buildCam.yaw = -90;

                if(j == 2 || j == 3) 
                    buildCam.yaw = -90;

                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, cubemapTex, 0);
                renderer->RenderWorld(buildCam, cubemapTex);
            }

            std::vector<unsigned char> buffer(resolution * resolution * 3);
            std::string basePath = cubemapDir + "cubemap_" + std::to_string(i);
            std::vector<std::string> faceNames = { "right", "left", "top", "bottom", "front", "back" };
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
            stbi_flip_vertically_on_write(true);

            for (int j = 0; j < 6; ++j)
            {
                glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
                std::string path = Filesystem::GetFullPath(basePath + "_" + faceNames[j] + ".png");
                stbi_write_png(path.c_str(), resolution, resolution, 3, buffer.data(), resolution * 3);
            }

            std::ofstream metaFile(Filesystem::GetFullPath(basePath + ".txt"));
            metaFile << origin.x << " " << origin.y << " " << origin.z << std::endl;
            metaFile << mins.x << " " << mins.y << " " << mins.z << std::endl;
            metaFile << maxs.x << " " << maxs.y << " " << maxs.z << std::endl;
            metaFile.close();

            glDeleteTextures(1, &cubemapTex);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &rbo);

        Console::Log("Cubemap build complete!");
        LoadForMap(mapName);
    }
}