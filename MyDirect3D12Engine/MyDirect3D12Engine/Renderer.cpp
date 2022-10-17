#include "Renderer.h"
using namespace Microsoft::WRL;

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
void Renderer::ConfigureViewportAndScissorRect()
{
	// Configure screen viewport
	mScreenViewport.Width = mViewportWidth;
	mScreenViewport.Height = mViewportHeight;
	mScreenViewport.TopLeftX = 0.0f;
	mScreenViewport.TopLeftY = 0.0f;
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	// Configure scissor rectangle
	mScissorRect.left = 0;
	mScissorRect.right = static_cast<LONG>(mViewportWidth);
	mScissorRect.top = 0;
	mScissorRect.bottom = static_cast<LONG>(mViewportHeight);
}
void Renderer::ConfigureInputElements()
{
	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void Renderer::CreateRootSignature(ID3D12Device* device, const std::string& rootSignatureName)
{
	RootSignature rootSignature = nullptr;

	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_ROOT_PARAMETER slotRootParameters[1];
	slotRootParameters[0].InitAsDescriptorTable(1, &cbvTable);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(5, slotRootParameters,
		0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSignature;
	ComPtr<ID3DBlob> error;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
		serializedRootSignature.GetAddressOf(), error.GetAddressOf());

	if (error != nullptr)
	{
		::OutputDebugStringA((char*)error->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(device->CreateRootSignature(1, serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	mRootSignatures.insert({ rootSignatureName, rootSignature });
}
void Renderer::CreatePSO(ID3D12Device* device, const std::string& psoName)
{
	PipelineStateObject pso = nullptr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = mDepthStencil->GetDepthStencilBufferFormat();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.NodeMask = 0;
	psoDesc.NumRenderTargets = 1;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.pRootSignature = mRootSignatures["default"].Get();
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaqueVS"]->GetShader()->GetBufferPointer()),
		mShaders["opaqueVS"]->GetShader()->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetShader()->GetBufferPointer()),
		mShaders["opaquePS"]->GetShader()->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RTVFormats[0] = mSwapChain->GetBackBufferFormat();
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.SampleMask = 0;
	
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pso.GetAddressOf())));
	
	mPSOs.insert({ psoName, pso });
}