#include "Renderer.h"
#include "Window.h"

int main()
{
    Renderer r;
    Window w{ 640, 480, &r };
    while (w.Run())
    {
    }
    return 0;
}
