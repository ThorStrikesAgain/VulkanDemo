#pragma once

#include <string>

namespace VulkanDemo
{
    class Object
    {
    public:
        Object();
        virtual ~Object();

        std::string const & GetName() const;
        void SetName(std::string const &name);

    private:
        Object(Object const &other) = delete;
        void operator=(Object const &other) = delete;

        std::string m_Name;
    };
} // VulkanDemo
