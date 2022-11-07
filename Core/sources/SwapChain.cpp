#include "../includes/SwapChain.h"

SwapChain::SwapChain()
{
	mBackBuffers.reserve(mBackBufferCount);
	for (UINT i = 0; i < mBackBufferCount; i++)
		mBackBuffers.push_back(nullptr);
}
SwapChain::SwapChain(UINT backBufferCount, DXGI_FORMAT backBufferFormat)
	: mBackBufferCount(backBufferCount), mBackBufferFormat(backBufferFormat)
{
	mBackBuffers.reserve(mBackBufferCount);
	for (UINT i = 0; i < mBackBufferCount; i++)
		mBackBuffers.push_back(nullptr);
}

void SwapChain::CreateSwapChain(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue, 
	HWND hWnd, UINT width, UINT height, 
	bool _4xMsaaState, UINT _4xMsaaQuality)
{
	// Release the previous swapchain we will be recreating.
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	swapChainDesc.BufferCount = mBackBufferCount;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.Format = mBackBufferFormat;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = _4xMsaaState ? 4 : 1;
	swapChainDesc.SampleDesc.Quality = _4xMsaaState ? (_4xMsaaQuality - 1) : 0;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Windowed = true;

	// Note: Swap chain uses queue to perform flush.
	ThrowIfFailed(factory->CreateSwapChain(commandQueue, 
		&swapChainDesc, mSwapChain.GetAddressOf()));

	for (UINT i = 0; i < mBackBufferCount; i++)
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i])));
}

void SwapChain::ResizeBackBuffers(UINT width, UINT height)
{
	for (UINT i = 0; i < mBackBufferCount; i++)
		mBackBuffers[i].Reset();

	mSwapChain->ResizeBuffers(
		mBackBufferCount,
		width,
		height,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	mCurrentBackBuffer = 0;

	for (UINT i = 0; i < mBackBufferCount; i++)
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i])));
}

IDXGISwapChain* SwapChain::GetSwapChain()
{
	return mSwapChain.Get();
}
UINT SwapChain::GetBackBufferCount()
{
	return mBackBufferCount;
}
DXGI_FORMAT SwapChain::GetBackBufferFormat()
{
	return mBackBufferFormat;
}
ID3D12Resource* SwapChain::GetBackBuffer(UINT backBufferIndex)
{
	if (backBufferIndex < 0 || backBufferIndex >= mBackBufferCount)
		return nullptr;

	return mBackBuffers[backBufferIndex].Get();
}
ID3D12Resource* SwapChain::GetCurrentBackBuffer()
{
	return mBackBuffers[mCurrentBackBuffer].Get();
}
UINT SwapChain::GetCurrentBackBufferIndex()
{
	return mCurrentBackBuffer;
}

void SwapChain::SwitchBackBuffer()
{
	mCurrentBackBuffer = (mCurrentBackBuffer + 1) % mBackBufferCount;
}
