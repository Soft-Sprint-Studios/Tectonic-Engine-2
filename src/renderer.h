#pragma once
#include "window.h"
#include "shader.h"
#include "camera.h"
#include "r_postprocess.h"
#include "r_bsp.h"
#include "r_models.h"
#include "r_sky.h"
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
    void OnWindowResize(int w, int h);

private:
    Window* m_windowRef;
    Shader m_worldShader;

    // Sub-renderers
    std::unique_ptr<R_PostProcess> m_postProcess;
    std::unique_ptr<R_BSP> m_bspRenderer;
    std::unique_ptr<R_Models> m_modelRenderer;
    std::unique_ptr<R_Sky> m_skyRenderer;
};