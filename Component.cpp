#include "Component.h"
#include "GameObject.h"

namespace VulkanDemo
{
    Component::Component() :
        m_GameObject{ nullptr }
    {
    }

    Component::~Component()
    {
        if (m_GameObject != nullptr)
        {
            m_GameObject->RemoveComponent(*this);
        }
    }
} // VulkanDemo
