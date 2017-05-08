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
} // VulkanDemo
