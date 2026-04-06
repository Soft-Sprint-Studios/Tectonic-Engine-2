#pragma once
#include "shader.h"
#include "camera.h"
#include <glad/glad.h>
#include <string>
#include <vector>

class R_Sky
{
public:
    R_Sky();
    ~R_Sky();

    bool Init(const std::string& skyName);
    void Draw(const Camera& camera);
    void Shutdown();

private:
    GLuint m_vao, m_vbo;
    GLuint m_cubemapTexture;
    Shader m_shader;

    void LoadCubemap(const std::string& skyName);
};