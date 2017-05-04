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

        glm::vec3 const & GetLocalPosition() const;
        void SetLocalPosition(glm::vec3 const & localPosition);

        glm::quat const & GetLocalRotation() const;
        void SetLocalRotation(glm::quat const & localRotation);

        glm::vec3 const & GetLocalScale() const;
        void SetLocalScale(glm::vec3 const & localScale);

    private:
        Transform(Transform const & other) = delete;
        void operator=(Transform const & other) = delete;

        glm::vec3 m_LocalPosition;
        glm::quat m_LocalRotation;
        glm::vec3 m_LocalScale;
    };
} // VulkanDemo
