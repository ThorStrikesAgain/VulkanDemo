#pragma once

#include <vector>

#include "Component.h"

namespace VulkanDemo
{
    class GameObject : public Object
    {
    public:
        GameObject();
        virtual ~GameObject();

        void AddComponent(Component& component);
        void RemoveComponent(const Component& component);
        std::vector<Component*> const & GetComponents() const;

        GameObject* GetParent() const;
        void SetParent(GameObject* parent);

        std::vector<GameObject*> const & GetChildren() const;

        ///
        /// Indicates whether the provided transform is strictly a descendant of this object. If the provided object is
        /// a descendant, the object on which this method is called shall be in its hierarchy of parents.
        ///
        bool IsDescendantOf(GameObject const & some) const;

    private:
        GameObject(GameObject const &other) = delete;
        void operator=(GameObject const &other) = delete;

        std::vector<Component*> m_Components;
        GameObject* m_Parent;
        std::vector<GameObject*> m_Children;
    };
} // VulkanDemo
