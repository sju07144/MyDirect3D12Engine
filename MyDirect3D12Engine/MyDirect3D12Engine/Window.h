#pragma once
#include "Utility.h"

LRESULT WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class Window
{
public:
	Window(HINSTANCE hInstance);
	Window(HINSTANCE hInstance, UINT width, UINT height);
	~Window();

	bool InitializeWindow();

	HWND GetWindowHandle();
	UINT GetWindowWidth();
	UINT GetWindowHeight();

	LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static Window* GetWindowPointer();
private:
	// Window size variables.
	UINT mWindowWidth;
	UINT mWindowHeight;

	// Window handle and instance.
	HWND mhWnd = nullptr;
	HINSTANCE mhInstance = nullptr;

	static Window* window;
};