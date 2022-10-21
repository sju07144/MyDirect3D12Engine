#pragma once
#include "Utility.h"

class SwapChain
{
public:
	SwapChain();
	SwapChain(UINT backBufferCount, DXGI_FORMAT backBufferFormat);

	void CreateSwapChain(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue, 
		HWND hWnd, UINT width, UINT height, 
		bool _4xMsaaState = false, UINT _4xMsaaQuality = 0);

	void ResizeBackBuffers(UINT width, UINT height);

	IDXGISwapChain* GetSwapChain();
	UINT GetBackBufferCount();
	DXGI_FORMAT GetBackBufferFormat();
	ID3D12Resource* GetBackBuffer(UINT backBufferIndex);
	ID3D12Resource* GetCurrentBackBuffer();
	UINT GetCurrentBackBufferIndex();

	void SwitchBackBuffer();
private:
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mBackBuffers;
	UINT mBackBufferCount = 2;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	UINT mCurrentBackBuffer = 0;
};