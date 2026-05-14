#include <std_includes.h>
#include "renderer/renderer.h"
#include "application/application.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	Renderer renderer(800, 600);
	return Application::Run(&renderer, hInstance, nCmdShow);
}
