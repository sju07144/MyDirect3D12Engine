#include "Renderer.h"

Renderer::Renderer(HINSTANCE hInstance)
	: mWindow(std::make_unique<Window>(hInstance, 800, 600)),
	mDirect3D(std::make_unique<BasicDirect3DComponent>()),
	mCommand(std::make_unique<Command>()),
	mSwapChain(std::make_unique<SwapChain>()),
	mDepthStencil(std::make_unique<DepthStencil>())
{
	mDescriptors[0] = std::make_unique<RtvDescriptor>(ResourceDimension::texture2D);
	mDescriptors[1] = std::make_unique<DsvDescriptor>(ResourceDimension::texture2D);
	mDescriptors[2] = std::make_unique<CbvDescriptor>(ResourceDimension::texture2D);
	mDescriptors[3] = std::make_unique<SrvDescriptor>(ResourceDimension::texture2D);
	mDescriptors[4] = std::make_unique<UavDescriptor>(ResourceDimension::texture2D);

	mViewportWidth = mWindow->GetWindowWidth();
	mViewportHeight = mWindow->GetWindowHeight();
}

void Renderer::Initialize()
{
	if (!mWindow->InitializeWindow())
	{
		MessageBox(0, L"Window creation failed!", nullptr, 0);
		return;
	}

	// Enable debug layer if debug mode enabled
#if defined(DEBUG) || defined(_DEBUG)
	EnableDebugLayer();
#endif

	// Create factory, device, fence
	mDirect3D->CreateBasicDirect3DComponent();

	CheckMultiSamplingSupport(mDirect3D->GetDevice(), mSwapChain->GetBackBufferFormat());

	// Create command objects
	mCommand->CreateCommandAllocator(mDirect3D->GetDevice());
	mCommand->CreateCommandQueue(mDirect3D->GetDevice());
	mCommand->CreateGraphicsCommandList(mDirect3D->GetDevice());

	// Create swap chain and back buffer
	mSwapChain->CreateSwapChain(mDirect3D->GetFactory(),
		mCommand->GetCommandQueue(), mWindow->GetWindowHandle(),
		mViewportWidth, mViewportHeight);

	// Create depth stencil buffer
	mDepthStencil->CreateDepthStencilBuffer(mDirect3D->GetDevice(), 
		mCommand->GetCommandList(),
		mViewportWidth, mViewportHeight, 
		m4xMsaaState, m4xMsaaQuality);

	// Create render target view
	mDescriptors[0]->CreateDescriptorHeap(mDirect3D->GetDevice(), mSwapChain->GetBackBufferCount());
	mDescriptors[0]->CreateDescriptor(mDirect3D->GetDevice(), mSwapChain->GetBackBufferFormat());

	// Create depth stencil view
	mDescriptors[1]->CreateDescriptorHeap(mDirect3D->GetDevice(), 1);
	mDescriptors[1]->CreateDescriptor(mDirect3D->GetDevice(), mDepthStencil->GetDepthStencilBufferFormat());

	ProcessMessages();
}

void Renderer::Render()
{
	
}

int Renderer::ProcessMessages()
{
	MSG msg = { 0 };

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

void Renderer::EnableDebugLayer()
{
	Microsoft::WRL::ComPtr<ID3D12Debug> debugLayer;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer)));
	debugLayer->EnableDebugLayer();
}
void Renderer::CheckMultiSamplingSupport(ID3D12Device* device, DXGI_FORMAT backBufferFormat)
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = backBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(device->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
}
