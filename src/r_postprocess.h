#pragma once
#include "shader.h"
#include <glad/glad.h>

class R_PostProcess
{
public:
    R_PostProcess();
    ~R_PostProcess();

    bool Init(int width, int height);
    void Begin();
    void End();
    void Draw();
    void Rescale(int width, int height);
    void Shutdown();

private:
    GLuint m_fbo;
    GLuint m_texture;
    GLuint m_rbo;

    GLuint m_msFbo;
    GLuint m_msTexture;
    GLuint m_msRbo;

    GLuint m_quadVAO;
    GLuint m_quadVBO;
    Shader m_shader;
    int m_width, m_height;

    void SetupBuffers();
};