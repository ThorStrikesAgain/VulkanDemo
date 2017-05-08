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
} // VulkanDemo
