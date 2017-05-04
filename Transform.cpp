#include "Transform.h"

#include <cassert>
#include <algorithm>

namespace VulkanDemo
{
    Transform::Transform() :
        m_LocalPosition{ 0, 0, 0 },
        m_LocalRotation{ 1, 0, 0, 0 },
        m_LocalScale{ 1, 1, 1 },
        m_Parent{ nullptr }
    {
    }

    Transform::~Transform()
    {
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

    Transform* Transform::GetParent() const
    {
        return m_Parent;
    }

    ///
    /// The parent doesn't take ownership of this transform.
    ///
    void Transform::SetParent(Transform* newParent)
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
            std::vector<Transform*> & childrenOfOldParent = m_Parent->m_Children;
            auto iter = std::find(childrenOfOldParent.begin(), childrenOfOldParent.end(), this);
            assert(iter != childrenOfOldParent.end()); // We should be in the list of children of our parent.
            childrenOfOldParent.erase(iter);
        }

        // Set the new parent.
        m_Parent = newParent;
    }

    std::vector<Transform*> const & Transform::GetChildren() const
    {
        return m_Children;
    }

    bool Transform::IsDescendantOf(Transform const & some) const
    {
        Transform const * toTest = m_Parent;
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
