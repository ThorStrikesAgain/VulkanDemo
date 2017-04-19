#include "GameObject.h"

#include <cassert>

namespace VulkanDemo
{
	GameObject::GameObject()
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

	std::vector<Component*> const & GameObject::GetComponents() const
	{
		return m_Components;
	}
} // VulkanDemo
