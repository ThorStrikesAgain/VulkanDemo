#include "Scene.h"

#include "GameObject.h"

namespace VulkanDemo
{
    Scene::Scene()
    {
    }

    Scene::~Scene()
    {
        for (auto gameObject : m_GameObjects)
        {
            delete gameObject;
        }
        m_GameObjects.clear();
    }

    void Scene::AddGameObjects(GameObject * gameObjects, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            m_GameObjects.insert(gameObjects + i);
        }
    }

    void Scene::RemoveGameObjects(GameObject * gameObjects, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            m_GameObjects.erase(gameObjects + i);
        }
    }
}
