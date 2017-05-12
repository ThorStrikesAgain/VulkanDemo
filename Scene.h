#pragma once

#include <unordered_set>

namespace VulkanDemo
{
    class GameObject;

    class Scene
    {
    public:
        Scene();
        ~Scene();

        ///
        /// The scene becomes the owner of the GameObjects.
        ///
        void AddGameObjects(GameObject * gameObjects, int count);

        ///
        /// The caller becomes the owner of the GameObjects.
        ///
        void RemoveGameObjects(GameObject * gameObjects, int count);

        ///
        /// Returns a reference to the internal list of  GameObjects.
        ///
        inline std::unordered_set<GameObject *> const & GetAllGameObjects() const { return m_GameObjects; }

    private:
        std::unordered_set<GameObject *> m_GameObjects;
    };
} // VulkanDemo
