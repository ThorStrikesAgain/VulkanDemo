#include "Renderer.h"

Renderer::Renderer()
{
    CreateInstance();
}

Renderer::~Renderer()
{
    DestroyInstance();
}

void Renderer::CreateInstance()
{
    VkApplicationInfo applicationInfo{};
    applicationInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName    = "VulkanDemo";
    applicationInfo.applicationVersion  = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.pEngineName         = "";
    applicationInfo.engineVersion       = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.apiVersion          = VK_MAKE_VERSION(1, 0, 42);

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo         = &applicationInfo;
    instanceCreateInfo.enabledLayerCount        = m_InstanceLayerNames.size();
    instanceCreateInfo.ppEnabledLayerNames      = m_InstanceLayerNames.data();
    instanceCreateInfo.enabledExtensionCount    = m_InstanceExtensionNames.size();
    instanceCreateInfo.ppEnabledExtensionNames  = m_InstanceExtensionNames.data();

    VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &m_Instance);
}

void Renderer::DestroyInstance()
{
    vkDestroyInstance(m_Instance, NULL);
    m_Instance = NULL;
}
