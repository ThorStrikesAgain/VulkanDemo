#include "VulkanManager.h"
#include "Window.h"
#include "WindowRenderer.h"

using namespace VulkanDemo;

int main()
{
    VulkanManager vm;
    Window w{ 640, 480, &vm };
    WindowRenderer wr{ &vm, &w };

    while (w.Run())
    {
        // TODO: Generate a dummy image.
        wr.Render(VK_NULL_HANDLE);
    }

    return 0;
}
