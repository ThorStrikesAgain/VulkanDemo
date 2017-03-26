#include "Renderer.h"

#include <cassert>

#include <array>
#include <iostream>

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
    DisplayInstanceLayers();
    DisplayInstanceExtensions();

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
    instanceCreateInfo.enabledLayerCount        = (uint32_t)m_InstanceLayerNames.size();
    instanceCreateInfo.ppEnabledLayerNames      = m_InstanceLayerNames.data();
    instanceCreateInfo.enabledExtensionCount    = (uint32_t)m_InstanceExtensionNames.size();
    instanceCreateInfo.ppEnabledExtensionNames  = m_InstanceExtensionNames.data();

    CheckResult(vkCreateInstance(&instanceCreateInfo, NULL, &m_Instance));
}

void Renderer::DestroyInstance()
{
    vkDestroyInstance(m_Instance, NULL);
    m_Instance = NULL;
}

void Renderer::DisplayInstanceLayers()
{
    uint32_t layerCount;
    CheckResult(vkEnumerateInstanceLayerProperties(&layerCount, NULL));
    std::vector<VkLayerProperties> availableLayers{ layerCount };
    CheckResult(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

    std::array<char *, 2> headers = { "[Name]", "[Description]" };
    std::cout << "Available Instance Layers:" << std::endl;
    PrintTable((int)headers.size(), layerCount, headers.data(), [availableLayers](int row, int col) {
        if (col == 0) return availableLayers[row].layerName;
        if (col == 1) return availableLayers[row].description;
        return "";
    });
    std::cout << std::endl;
}

void Renderer::DisplayInstanceExtensions()
{
    uint32_t extensionCount;
    CheckResult(vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL));
    std::vector<VkExtensionProperties> availableExtensions{ extensionCount };
    CheckResult(vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, availableExtensions.data()));

    std::cout << "Available Instance Extensions:" << std::endl;
    PrintTable(1, extensionCount, nullptr, [availableExtensions](int row, int col) {
        return availableExtensions[row].extensionName;
    });
    std::cout << std::endl;
}
