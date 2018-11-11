#include "UIRenderer.h"

#include "Window.h"
#include "VulkanManager.h"
#include "Application.h"

#include "external/dear-imgui/imgui.h"
#include "external/dear-imgui/imgui_impl_vulkan.h"
#include "external/dear-imgui/imgui_impl_win32.h"

VulkanDemo::UIRenderer::UIRenderer(Window * window) :
    m_Window(window)
{
    m_VulkanManager = Application::GetInstance().GetVulkanManager();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplWin32_Init(m_Window->GetWindowHandle());

    ImGui_ImplVulkan_InitInfo vulkanInitInfo = { 0 };
    vulkanInitInfo.Instance = m_VulkanManager->GetInstance();
    vulkanInitInfo.PhysicalDevice = m_VulkanManager->GetPhysicalDevice();
    vulkanInitInfo.Device = m_VulkanManager->GetDevice();
    vulkanInitInfo.QueueFamily = m_VulkanManager->GetGraphicsQueueFamilyIndex();
    vulkanInitInfo.Queue = m_VulkanManager->GetGraphicsQueue();
    vulkanInitInfo.PipelineCache = VK_NULL_HANDLE;
    vulkanInitInfo.DescriptorPool = m_VulkanManager->GetDescriptorPool();
    vulkanInitInfo.Allocator = NULL;
    vulkanInitInfo.CheckVkResultFn = CheckResult;

    ImGui_ImplVulkan_Init(&vulkanInitInfo, window->GetRenderPass());

    ImGui::StyleColorsDark();
}

VulkanDemo::UIRenderer::~UIRenderer()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void VulkanDemo::UIRenderer::BeginNewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f; // TODO!
    io.DisplaySize.x = (float)m_Window->GetWidth();
    io.DisplaySize.y = (float)m_Window->GetHeight();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();

    static bool init = false;
    if (init)
        ImGui::NewFrame();
    else
        init = true;
}

void VulkanDemo::UIRenderer::Draw(VkCommandBuffer commandBuffer)
{
    // TODO: THIS MUST NOT BE STATIC
    static bool init = false;
    if (!init)
    {
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        ImGui_ImplVulkan_InvalidateFontUploadObjects();
        ImGui::NewFrame();
        init = true;
    }

    ImGui::Begin("Hello, world!");

    ImGui::Text("This is some useful text.");
    ImGui::Button("Button");

    ImGui::End();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

bool VulkanDemo::UIRenderer::ProcessEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT * result)
{
    if (ImGui::GetCurrentContext() == NULL)
        return false;

    ImGuiIO& io = ImGui::GetIO();

    switch (uMsg)
    {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    {
        int button = 0;
        if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK) button = 0;
        if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONDBLCLK) button = 1;
        if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONDBLCLK) button = 2;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
            ::SetCapture(hwnd);
        io.MouseDown[button] = true;
        *result = 0;
        return true;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
        int button = 0;
        if (uMsg == WM_LBUTTONUP) button = 0;
        if (uMsg == WM_RBUTTONUP) button = 1;
        if (uMsg == WM_MBUTTONUP) button = 2;
        io.MouseDown[button] = false;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
            ::ReleaseCapture();
        *result = 0;
        return true;
    }
    case WM_MOUSEWHEEL:
        io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        *result = 0;
        return true;
    case WM_MOUSEHWHEEL:
        io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        *result = 0;
        return true;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam < 256)
            io.KeysDown[wParam] = 1;
        *result = 0;
        return true;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wParam < 256)
            io.KeysDown[wParam] = 0;
        *result = 0;
        return true;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacter((unsigned short)wParam);
        *result = 0;
        return true;
    /*case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && ImGui_ImplWin32_UpdateMouseCursor())
            *result = 1;
        return false;*/
    }

    return false;
}
