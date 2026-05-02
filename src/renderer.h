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
#include "window.h"
#include "r_shader.h"
#include "camera.h"
#include "r_postprocess.h"
#include "r_bsp.h"
#include "r_models.h"
#include "r_sky.h"
#include "r_particles.h"
#include "r_lights.h"
#include "r_sprites.h"
#include "r_waters.h"
#include "r_ui.h"
#include "r_state.h"
#include "r_beams.h"
#include "cubemap.h"
#include <memory>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Init(Window& window);
    bool LoadMap(const std::string& path);
    void Shutdown();
    void Render(Camera& camera);
    void RenderWorld(Camera& camera, GLuint cubemapToExclude = 0, bool drawWater = true);
    static void DrawSceneDepth(R_Shader& shader, const Frustum& frustum, R_BSP* bsp, R_Models* models);
    void OnWindowResize(int w, int h);

    R_UI* GetUI() const 
    { 
        return m_uiRenderer.get(); 
    }

private:
    bool m_drawingWater = false;
    void DrawWorld(Camera& camera, GLuint cubemapToExclude, bool drawWater);
    void DrawPrePass(Camera& camera);
    Window* m_windowRef;
    R_Shader m_worldShader;
    R_Shader m_depthShader;

    // Sub-renderers
    std::unique_ptr<R_PostProcess> m_postProcess;
    std::unique_ptr<R_BSP> m_bspRenderer;
    std::unique_ptr<R_Models> m_modelRenderer;
    std::unique_ptr<R_Sky> m_skyRenderer;
    std::unique_ptr<R_Particles> m_particleRenderer;
    std::unique_ptr<R_Lights> m_lightRenderer;
    std::unique_ptr<R_Sprites> m_spriteRenderer;
    std::unique_ptr<R_Beams> m_beamRenderer;
    std::unique_ptr<R_Waters> m_waterRenderer;
    std::unique_ptr<R_UI> m_uiRenderer;
};