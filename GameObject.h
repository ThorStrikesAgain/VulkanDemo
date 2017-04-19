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

	private:
		GameObject(GameObject const &other) = delete;
		void operator=(GameObject const &other) = delete;

		std::vector<Component*> m_Components;
	};
} // VulkanDemo
