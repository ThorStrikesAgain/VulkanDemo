#pragma once

#include "Component.h"

#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace VulkanDemo
{
    class Transform : public Component
    {
    public:
        Transform();
        virtual ~Transform();

        inline glm::vec3 const & GetLocalPosition() const { return m_LocalPosition; }
        inline void SetLocalPosition(glm::vec3 const & localPosition) { m_LocalPosition = localPosition; }

        inline glm::quat const & GetLocalRotation() const { return m_LocalRotation; }
        inline void SetLocalRotation(glm::quat const & localRotation) { m_LocalRotation = localRotation; }

        inline glm::vec3 const & GetLocalScale() const { return m_LocalScale; }
        inline void SetLocalScale(glm::vec3 const & localScale) { m_LocalScale = localScale; }

    private:
        Transform(Transform const & other) = delete;
        void operator=(Transform const & other) = delete;

        glm::vec3 m_LocalPosition;
        glm::quat m_LocalRotation;
        glm::vec3 m_LocalScale;
    };
} // VulkanDemo
