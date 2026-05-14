#pragma once
//TODO:review includes
#include "../renderer/renderer.h"

class Application
{
public:
	static int Run(Renderer* demo, HINSTANCE hInstance, int nCmdShow);

	static HWND GetHwnd();

private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static HWND m_hwnd;

	static void InitializeWindowClass(HINSTANCE hInst, const wchar_t* windowClassName, WNDCLASSEXW* outWndClass);

	static HWND CreateApplicationWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, RECT& windowRect, Renderer* renderer);

};
