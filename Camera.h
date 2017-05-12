#pragma once

#include "Component.h"

namespace VulkanDemo
{
    class Camera : public Component
    {
    public:
        Camera();
        ~Camera();

        inline float GetNear() const { return m_Near; }
        inline void SetNear(float near) { m_Near = near; }

        inline float GetFar() const { return m_Far; }
        inline void SetFar(float far) { m_Far = far; }

        inline float GetVerticalFieldOfView() const { return m_VerticalFieldOfView; }
        inline void SetVerticalFieldOfView(float vfov) { m_VerticalFieldOfView = vfov; }

    private:
        float m_Near = 0.1f;
        float m_Far = 100;
        float m_VerticalFieldOfView = 45; // In degrees.
    };
} // VulkanDemo
