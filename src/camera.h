#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

private:
    float m_fov, m_aspect, m_near, m_far;
};