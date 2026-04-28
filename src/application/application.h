#pragma once
//TODO:review includes
#include "../demo/demo.h"

class Application
{
public:
	static int Run(Demo* demo, HINSTANCE hInstance, int nCmdShow);

private:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	static HWND m_hwnd;

	static void InitializeWindowClass(HINSTANCE hInst, const wchar_t* windowClassName, WNDCLASSEXW* outWndClass);

	static HWND CreateApplicationWindow(const wchar_t* windowClassName, HINSTANCE hInst, const wchar_t* windowTitle, uint32_t width, uint32_t height);

};
