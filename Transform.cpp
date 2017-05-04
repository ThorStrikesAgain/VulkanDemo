#include "Transform.h"

#include <cassert>
#include <algorithm>

namespace VulkanDemo
{
    Transform::Transform() :
        m_LocalPosition{ 0, 0, 0 },
        m_LocalRotation{ 1, 0, 0, 0 },
        m_LocalScale{ 1, 1, 1 }
    {
    }

    Transform::~Transform()
    {
    }

    glm::vec3 const & Transform::GetLocalPosition() const
    {
        return m_LocalPosition;
    }

    void Transform::SetLocalPosition(const glm::vec3& localPosition)
    {
        m_LocalPosition = localPosition;
    }

    glm::quat const & Transform::GetLocalRotation() const
    {
        return m_LocalRotation;
    }

    void Transform::SetLocalRotation(const glm::quat& localRotation)
    {
        m_LocalRotation = localRotation;
    }

    glm::vec3 const & Transform::GetLocalScale() const
    {
        return m_LocalScale;
    }

    void Transform::SetLocalScale(const glm::vec3& localScale)
    {
        m_LocalScale = localScale;
    }
} // VulkanDemo
