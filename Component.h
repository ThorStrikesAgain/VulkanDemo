#pragma once

#include "Object.h"

namespace VulkanDemo
{
    class GameObject;

    class Component : public Object
    {
        friend GameObject;

    public:
        Component();
        virtual ~Component();

        inline GameObject* GetGameObject() const { return m_GameObject; }

    private:
        Component(Component const &other) = delete;
        void operator=(Component const &other) = delete;

        GameObject* m_GameObject;
    };
} // VulkanDemo
