#include "Renderer.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdline, int nCmdShow)
{
	Renderer renderer(hInstance);

	try
	{
		renderer.Initialize();

		return renderer.RenderLoop();
	}
	catch (const DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return -1;
	}
	catch (const std::runtime_error& e)
	{
		::OutputDebugStringA(e.what());
		return -1;
	}

	return 0;
}