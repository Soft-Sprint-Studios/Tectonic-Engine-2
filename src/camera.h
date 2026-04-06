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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Frustum 
{
    glm::vec4 planes[6];

    bool IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const 
    {
        for (int i = 0; i < 6; i++) 
        {
            glm::vec3 p = min;
            if (planes[i].x >= 0) 
                p.x = max.x;
            if (planes[i].y >= 0) 
                p.y = max.y;
            if (planes[i].z >= 0) 
                p.z = max.z;

            if (glm::dot(glm::vec3(planes[i]), p) + planes[i].w < 0)
                return false;
        }
        return true;
    }
};

class Camera {
public:
    Camera(float fov = 75.0f, float aspect = 16.0f/9.0f, float nearP = 0.1f, float farP = 1000.0f)
        : m_fov(fov), m_aspect(aspect), m_near(nearP), m_far(farP) {}

    glm::vec3 position{ 0.0f, 0.0f, 5.0f };
    float yaw = -90.0f;
    float pitch = 0.0f;

    glm::vec3 GetForward() const 
    {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::normalize(front);
    }

    glm::mat4 GetViewMatrix() const 
    {
        return glm::lookAt(position, position + GetForward(), glm::vec3(0, 1, 0));
    }

    glm::mat4 GetProjectionMatrix() const 
    {
        return glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
    }

    void SetAspectRatio(float ratio) 
    { 
        m_aspect = ratio; 
    }

    float GetAspectRatio()
    {
        return m_aspect;
    }

    void SetFOV(float fov) 
    {
        m_fov = fov; 
    }

    float GetFOV() const 
    { 
        return m_fov; 
    }

    Frustum GetFrustum() const 
    {
        Frustum f;
        glm::mat4 m = GetProjectionMatrix() * GetViewMatrix();
        f.planes[0] = glm::vec4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]); // Left
        f.planes[1] = glm::vec4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]); // Right
        f.planes[2] = glm::vec4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]); // Bottom
        f.planes[3] = glm::vec4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]); // Top
        f.planes[4] = glm::vec4(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]); // Near
        f.planes[5] = glm::vec4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]); // Far

        for (int i = 0; i < 6; i++) 
        {
            float length = glm::length(glm::vec3(f.planes[i]));
            f.planes[i] /= length;
        }
        return f;
    }

private:
    float m_fov, m_aspect, m_near, m_far;
};