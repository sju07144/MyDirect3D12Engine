#include "Window.h"

Window* Window::window = nullptr;

LRESULT WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return Window::GetWindowPointer()->WndProc(hWnd, msg, wParam, lParam);
}

Window::Window(HINSTANCE hInstance)
	: mhInstance(hInstance), mWindowWidth(800), mWindowHeight(600)
{
	window = this;
}
Window::Window(HINSTANCE hInstance, UINT width, UINT height)
	: mhInstance(hInstance), mWindowWidth(width), mWindowHeight(height)
{
	window = this;
}
Window::~Window()
{
	window = nullptr;
}

bool Window::InitializeWindow()
{
	WNDCLASS wndClass;
	wndClass.lpszClassName = L"MainWindowClass";
	wndClass.lpszMenuName = nullptr;
	wndClass.lpfnWndProc = WindowProcedure;
	wndClass.hInstance = mhInstance;
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.cbWndExtra = 0;
	wndClass.cbClsExtra = 0;

	if (!RegisterClass(&wndClass))
	{
		MessageBox(0, L"Register class Failed.", 0, 0);
		return false;
	}

	RECT R = { 0, 0, mWindowWidth, mWindowHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhWnd = CreateWindow(
		L"MainWindowClass",
		L"MainWindow",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		0,
		0,
		mhInstance,
		0);
	if (!mhWnd)
	{
		MessageBox(nullptr, L"Create window failed!", nullptr, 0);
		return false;
	}

	ShowWindow(mhWnd, SW_SHOW);
	UpdateWindow(mhWnd);

	return true;
}

HWND Window::GetWindowHandle()
{
	return mhWnd;
}
UINT Window::GetWindowWidth()
{
	return mWindowWidth;
}
UINT Window::GetWindowHeight()
{
	return mWindowHeight;
}

LRESULT Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

Window* Window::GetWindowPointer()
{
	return window;
}
