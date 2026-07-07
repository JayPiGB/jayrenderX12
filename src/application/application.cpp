#include <std_includes.h>

#include "application.h"

HWND Application::m_hwnd = nullptr;

HWND Application::GetHwnd()
{
	return m_hwnd;
}

int Application::Run(Renderer *renderer, HINSTANCE hInstance, int nCmdShow)
{
	renderer->ParseCommandLineArgs();

	WNDCLASSEXW windowClass{};
	InitializeWindowClass(hInstance, L"JayrenderX12 window class", &windowClass);
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(renderer->GetWidth()), static_cast<LONG>(renderer->GetHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	m_hwnd = CreateApplicationWindow(windowClass.lpszClassName, hInstance, L"JayrenderX12", windowRect, renderer);

	renderer->OnInit();

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

	renderer->OnDestroy();

	return static_cast<char>(msg.wParam);
}

void Application::InitializeWindowClass(HINSTANCE hInst, const wchar_t* windowClassName, WNDCLASSEXW* windowClass)
{
	windowClass->cbSize = sizeof(WNDCLASSEX);
	windowClass->style = CS_HREDRAW | CS_VREDRAW;
	windowClass->lpfnWndProc = &WindowProc;
	windowClass->hInstance = hInst;
	windowClass->hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass->lpszClassName = windowClassName;
}

HWND Application::CreateApplicationWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, RECT& windowRect, Renderer* renderer) {
	return CreateWindow(
		windowClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		hInst,
		renderer
	);
}

LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	Renderer* renderer = reinterpret_cast<Renderer*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

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
			if (renderer)
			{
				renderer->OnUpdate();
				renderer->OnRender();
			}
			return 0;
			
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}
