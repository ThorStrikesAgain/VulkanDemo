#pragma once

#include <string>

namespace VulkanDemo
{
    class Object
    {
    public:
        Object();
        virtual ~Object();

        inline std::string const & GetName() const { return m_Name; }
        inline void SetName(std::string const &name) { m_Name = name; }

    private:
        Object(Object const &other) = delete;
        void operator=(Object const &other) = delete;

        std::string m_Name;
    };
} // VulkanDemo
