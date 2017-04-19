#include "Renderer.h"
#include "Window.h"
#include "WindowRenderer.h"

using namespace VulkanDemo;

int main()
{
    Renderer r;
    Window w{ 640, 480, &r };
    WindowRenderer wr{ &r, &w };

    while (w.Run())
    {
        wr.BeginFrame();
        wr.EndFrame();
    }

    return 0;
}
