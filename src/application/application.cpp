#include <std_includes.h>

#include "application.h"
#define UNICODE

HWND Application::m_hwnd = nullptr;
int Application::Run(Demo *demo, HINSTANCE hInstance, int nCmdShow)
{
	demo->ParseCommandLineArgs();

	WNDCLASSEXW windowClass{};
	InitializeWindowClass(hInstance, L"JayrenderX12 window class", &windowClass);
	RegisterClassExW(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(demo->GetWidth()), static_cast<LONG>(demo->GetHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	m_hwnd = CreateApplicationWindow(windowClass.lpszClassName, hInstance, L"JayrenderX12", demo->GetWidth(), demo->GetHeight());

	demo->OnInit();

	ShowWindow(m_hwnd, nCmdShow);

	MSG msg{};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	demo->OnDestroy();

	return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Demo* demo = reinterpret_cast<Demo*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch(message)
	{
		case WM_CREATE:
			{
				LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
			}
			return 0;
		case WM_KEYDOWN:
		case WM_PAINT:
			if (demo)
			{
				demo->OnUpdate();
				demo->OnRender();
			}
			return 0;
			
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
