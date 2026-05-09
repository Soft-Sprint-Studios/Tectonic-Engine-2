#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include "r_shader.h"
#include "camera.h"

#include <cstdio>
#include "pl_mpeg.h"

class R_VideoInstance
{
public:
    R_VideoInstance();
    ~R_VideoInstance();

    bool Load(const std::string& path, bool loop);
    void Update(float dt);
    void BindTextures();
    void Shutdown();

private:
    plm_t* m_plm = nullptr;
    GLuint m_texY = 0, m_texCb = 0, m_texCr = 0;
    bool m_loop = false;
    double m_time = 0.0;
    double m_nextFrameTime = 0.0;
};

class R_Video
{
public:
    void Init();
    void Draw(const Camera& camera, const std::vector<std::shared_ptr<class Video>>& videos);
    void Shutdown();

private:
    R_Shader m_shader;
    GLuint m_vao = 0, m_vbo = 0;
};