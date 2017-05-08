#include "GameObject.h"

#include <cassert>

namespace VulkanDemo
{
    GameObject::GameObject() : 
        m_Parent{ nullptr }
    {
    }

    GameObject::~GameObject()
    {
        for (auto component : m_Components)
        {
            component->m_GameObject = nullptr;
            delete component;
        }
        m_Components.clear();

        SetParent(nullptr);
        for (auto child : m_Children)
        {
            // If the user wants a more fancy behaviour (ex: set their parent to mine), he can implement it before
            // destroying the objects. But by default, we don't want to alter the parent. There could be side
            // effects unknown at this level, and we don't want to force the user to undo what we did.
            //
            // Don't call SetParent(nullptr) on the child from here, because it will attempt to modify children_
            // while we are
            // iterating over it.
            child->m_Parent = nullptr;
        }
        m_Children.clear();
    }

    ///
    /// The GameObject takes ownership of the component.
    ///
    void GameObject::AddComponent(Component& component)
    {
        m_Components.push_back(&component);
        assert(component.m_GameObject == nullptr);
        component.m_GameObject = this;
    }

    ///
    /// The GameObject denies ownership of the component.
    ///
    void GameObject::RemoveComponent(const Component& component)
    {
        auto iter = std::find(m_Components.begin(), m_Components.end(), &component);
        if (iter != m_Components.end())
        {
            assert((*iter)->m_GameObject == this);
            (*iter)->m_GameObject = nullptr;
            m_Components.erase(iter);
        }
    }

    ///
    /// The parent doesn't take ownership of this game object.
    ///
    void GameObject::SetParent(GameObject* newParent)
    {
        if (newParent == m_Parent)
        {
            return;
        }

        if (newParent != nullptr)
        {
            // Prevent loops.
            if (newParent == this || newParent->IsDescendantOf(*this))
            {
                assert(false); // TODO: Log an error instead of asserting.
                return;
            }

            // Add this to the children of the new parent.
            newParent->m_Children.push_back(this);
        }

        // Remove this from the children of the old parent.
        if (m_Parent != nullptr)
        {
            std::vector<GameObject*> & childrenOfOldParent = m_Parent->m_Children;
            auto iter = std::find(childrenOfOldParent.begin(), childrenOfOldParent.end(), this);
            assert(iter != childrenOfOldParent.end()); // We should be in the list of children of our parent.
            childrenOfOldParent.erase(iter);
        }

        // Set the new parent.
        m_Parent = newParent;
    }

    bool GameObject::IsDescendantOf(GameObject const & some) const
    {
        GameObject const * toTest = m_Parent;
        while (toTest != nullptr)
        {
            if (toTest == &some)
            {
                return true;
            }
            toTest = toTest->m_Parent;
        }

        return false;
    }
} // VulkanDemo
