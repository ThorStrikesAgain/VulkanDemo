#include "Object.h"

#include <sstream>

namespace VulkanDemo
{
    Object::Object()
    {
        static int sInstanceCounter = 0;
        std::ostringstream oss;
        oss << "Object" << sInstanceCounter++;
        m_Name = oss.str();
    }

    Object::~Object()
    {
    }

    std::string const & Object::GetName() const
    {
        return m_Name;
    }

    void Object::SetName(std::string const &name)
    {
        m_Name = name;
    }
} // VulkanDemo
