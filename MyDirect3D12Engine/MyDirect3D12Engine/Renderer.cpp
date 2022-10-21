#include "Renderer.h"
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

Renderer* Renderer::renderer = nullptr;

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return Renderer::GetRendererPointer()->WndProc(hWnd, msg, wParam, lParam);
}

Renderer::Renderer(HINSTANCE hInstance)
	: mWindowWidth(800), mWindowHeight(600),
	mViewportWidth(800), mViewportHeight(600),
	mDirect3D(std::make_unique<BasicDirect3DComponent>()),
	mCommand(std::make_unique<Command>()),
	mSwapChain(std::make_unique<SwapChain>()),
	mDepthStencil(std::make_unique<DepthStencil>()),
	mCamera(std::make_unique<Camera>()),
	mTimer(std::make_unique<Timer>())
{
	renderer = this;

	mDescriptors[DescriptorType::rtv] = std::make_unique<RtvDescriptor>(ResourceDimension::texture2D);
	mDescriptors[DescriptorType::dsv] = std::make_unique<DsvDescriptor>(ResourceDimension::texture2D);
	mDescriptors[DescriptorType::cbv] = std::make_unique<CbvDescriptor>(ResourceDimension::texture2D);
	mDescriptors[DescriptorType::srv] = std::make_unique<SrvDescriptor>(ResourceDimension::texture2D);
	mDescriptors[DescriptorType::uav] = std::make_unique<UavDescriptor>(ResourceDimension::texture2D);

	float aspectRatio = static_cast<float>(mViewportWidth) / static_cast<float>(mViewportHeight);

	mCamera->LookAt(
		6.0f, 8.0f, 6.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f);

	mCamera->SetLens(0.25f * MathUtility::PI, aspectRatio, 1.0f, 1000.0f);
}
Renderer::~Renderer()
{
	renderer = nullptr;
}

void Renderer::Initialize()
{
	if (!InitializeWindow())
	{
		MessageBox(0, L"Window creation failed!", nullptr, 0);
		return;
	}

	// Enable debug layer if debug mode enabled
#if defined(DEBUG) || defined(_DEBUG)
	EnableDebugLayer();
#endif
	// Configure viewport and scissor rectangle
	ConfigureViewportAndScissorRect();

	// Create factory, device, fence
	mDirect3D->CreateBasicDirect3DComponent();

	// Cache the objects
	auto device = mDirect3D->GetDevice();
	auto backBufferFormat = mSwapChain->GetBackBufferFormat();

	CheckMultiSamplingSupport(device, backBufferFormat);

	// Create command objects
	mCommand->CreateCommandAllocator(device);
	mCommand->CreateCommandQueue(device);
	mCommand->CreateGraphicsCommandList(device);

	auto commandList = mCommand->GetCommandList();
	auto commandQueue = mCommand->GetCommandQueue();

	// Create swap chain and back buffer
	mSwapChain->CreateSwapChain(mDirect3D->GetFactory(),
		mCommand->GetCommandQueue(), mhWnd,
		mViewportWidth, mViewportHeight);

	// Create depth stencil buffer
	mDepthStencil->CreateDepthStencilBuffer(device,
		mViewportWidth, mViewportHeight, 
		m4xMsaaState, m4xMsaaQuality);

	// Create render target view
	mDescriptors[DescriptorType::rtv]->CreateDescriptorHeap(device, mSwapChain->GetBackBufferCount());
	for (UINT i = 0; i < mSwapChain->GetBackBufferCount(); i++)
		mDescriptors[DescriptorType::rtv]->CreateDescriptor(device, mDirect3D->GetRtvDescriptorSize(),
			backBufferFormat, mSwapChain->GetBackBuffer(i));

	// Create depth stencil view
	mDescriptors[DescriptorType::dsv]->CreateDescriptorHeap(device, 1);
	mDescriptors[DescriptorType::dsv]->CreateDescriptor(device, mDirect3D->GetDsvDescriptorSize(),
		mDepthStencil->GetDepthStencilBufferFormat(), mDepthStencil->GetDepthStencilBuffer());

	// Create box
	BasicGeometryGenerator geoGenerator;
	std::unique_ptr<Mesh> box = std::make_unique<Mesh>(geoGenerator.CreateBox(2.0f, 2.0f, 2.0f));
	box->ConfigureMesh(device, commandList);
	mMeshes.insert({ "box", std::move(box) });

	// Initialize constant buffer
	mObjectCBs = std::make_unique<UploadBuffer<ObjectConstant>>(device, 1, true);

	// Create descriptor heap and constant buffer view
	mDescriptors[DescriptorType::cbv]->CreateDescriptorHeap(device, 1);
	mDescriptors[DescriptorType::cbv]->CreateDescriptor(device, mDirect3D->GetCbvSrvUavDescriptorSize(),
		DXGI_FORMAT_UNKNOWN, mObjectCBs->GetUploadBuffer(), nullptr, mObjectCBs->GetElementByteSize());

	// Create vertex and pixel shader
	std::unique_ptr<Shader> vertexShader = std::make_unique<Shader>();
	std::unique_ptr<Shader> pixelShader = std::make_unique<Shader>();
	vertexShader->CompileShader(L"./Shaders/opaque.hlsl", nullptr, "VSMain", "vs_5_1");
	pixelShader->CompileShader(L"./Shaders/opaque.hlsl", nullptr, "PSMain", "ps_5_1");
	mShaders.insert({ "opaqueVS", std::move(vertexShader) });
	mShaders.insert({ "opaquePS", std::move(pixelShader) });

	ConfigureInputElements();
	CreateRootSignature(device, "default");
	CreatePSO(device, "opaque", "default", "opaque");

	ExecuteCommandLists(commandList, commandQueue);

	mDirect3D->WaitForPreviousFrame(commandQueue);
}

int Renderer::RenderLoop()
{
	MSG msg = { 0 };

	mTimer->Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			mTimer->Tick();

			if (!mAppPaused)
			{
				ProcessKeyboardInput();
				UpdateData();
				DrawScene();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK Renderer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer->Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer->Start();
		}
		return 0;
	case WM_SIZE:
		mWindowWidth = LOWORD(lParam);
		mWindowHeight = HIWORD(lParam);
		mViewportWidth = LOWORD(lParam);
		mViewportHeight = HIWORD(lParam);

		if (mDirect3D->GetDevice())
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				Resize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					Resize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					Resize();
				}
				else if (mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					Resize();
				}
			}
		}
		return 0;

	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer->Stop();
		return 0;

	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer->Start();
		Resize();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

	case WM_GETMINMAXINFO:
		reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.x = 200;
		reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		MouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		MouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		MouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}

		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

Renderer* Renderer::GetRendererPointer()
{
	return renderer;
}

bool Renderer::InitializeWindow()
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

	RECT R = { 0, 0, static_cast<LONG>(mWindowWidth), static_cast<LONG>(mWindowHeight) };
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

void Renderer::Resize()
{
	auto device = mDirect3D->GetDevice();
	auto commandAllocator = mCommand->GetCommandAllocator();
	auto commandList = mCommand->GetCommandList();
	auto commandQueue = mCommand->GetCommandQueue();

	mDirect3D->WaitForPreviousFrame(commandQueue);

	// Resize back buffer
	mSwapChain->ResizeBackBuffers(mWindowWidth, mWindowHeight);
	
	mDescriptors[DescriptorType::rtv]->ResetDescriptorHeap();
	for (UINT i = 0; i < mSwapChain->GetBackBufferCount(); i++)
		mDescriptors[DescriptorType::rtv]->CreateDescriptor(
			mDirect3D->GetDevice(), mDirect3D->GetRtvDescriptorSize(),
			mSwapChain->GetBackBufferFormat(), mSwapChain->GetBackBuffer(i));

	// Resize depth/stencil buffer
	mDepthStencil->ResetDepthStencilBuffer();
	mDepthStencil->CreateDepthStencilBuffer(mDirect3D->GetDevice(),
		mWindowWidth, mWindowHeight, m4xMsaaState, m4xMsaaQuality);

	mDescriptors[DescriptorType::dsv]->ResetDescriptorHeap();
	mDescriptors[DescriptorType::dsv]->CreateDescriptor(
		mDirect3D->GetDevice(), mDirect3D->GetDsvDescriptorSize(),
		mDepthStencil->GetDepthStencilBufferFormat(), mDepthStencil->GetDepthStencilBuffer());

	ConfigureViewportAndScissorRect();
}

void Renderer::ExecuteCommandLists(ID3D12GraphicsCommandList* commandList, 
	ID3D12CommandQueue* commandQueue)
{
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* commandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void Renderer::MouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhWnd);
}
void Renderer::MouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}
void Renderer::MouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		mCamera->RotateX(dy);
		mCamera->RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void Renderer::ProcessKeyboardInput()
{
	const float dt = mTimer->DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera->Walk(10.0f * dt);
	
	if (GetAsyncKeyState('S') & 0x8000)
		mCamera->Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera->Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera->Strafe(10.0f * dt);
}
void Renderer::UpdateData()
{
	ObjectConstant objConstant;
	XMStoreFloat4x4(&objConstant.world, XMMatrixIdentity());
	objConstant.view = mCamera->GetView();
	objConstant.proj = mCamera->GetProj();

	mObjectCBs->CopyData(0, objConstant);
}
void Renderer::DrawScene()
{
	auto device = mDirect3D->GetDevice();
	auto commandAllocator = mCommand->GetCommandAllocator();
	auto commandList = mCommand->GetCommandList();
	auto commandQueue = mCommand->GetCommandQueue();

	auto currentBackBuffer = mSwapChain->GetCurrentBackBuffer();

	ThrowIfFailed(commandAllocator->Reset());

	ThrowIfFailed(commandList->Reset(commandAllocator, mPSOs["opaque"].Get()));

	PIXBeginEvent(commandQueue, 120, "OK");
	PIXBeginEvent(commandList, 120, "OK");

	commandList->RSSetScissorRects(1, &mScissorRect);
	commandList->RSSetViewports(1, &mScreenViewport);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer,
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_CPU_DESCRIPTOR_HANDLE currentRenderTargetView =
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptors[DescriptorType::rtv]->GetStartCPUDescriptorHandle(), 
		mSwapChain->GetCurrentBackBufferIndex(), mDirect3D->GetRtvDescriptorSize());
	commandList->ClearRenderTargetView(currentRenderTargetView, Colors::LightSteelBlue, 0, nullptr);
	commandList->ClearDepthStencilView(mDescriptors[DescriptorType::dsv]->GetStartCPUDescriptorHandle(), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	commandList->OMSetRenderTargets(1, &currentRenderTargetView, false, &mDescriptors[DescriptorType::dsv]->GetStartCPUDescriptorHandle());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mDescriptors[DescriptorType::cbv]->GetDescriptorHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootSignature(mRootSignatures["default"].Get());

	auto vbv = mMeshes["box"]->GetVertexBufferView();
	auto ibv = mMeshes["box"]->GetIndexBufferView();
	commandList->IASetVertexBuffers(0, 1, &vbv);
	commandList->IASetIndexBuffer(&ibv);
	commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetGraphicsRootDescriptorTable(0, mDescriptors[DescriptorType::cbv]->GetStartGPUDescriptorHandle());

	commandList->DrawIndexedInstanced(mMeshes["box"]->GetIndexCount(), 1, 0, 0, 0);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ExecuteCommandLists(commandList, commandQueue);

	ThrowIfFailed(mSwapChain->GetSwapChain()->Present(0, 0));
	mSwapChain->SwitchBackBuffer();

	mDirect3D->WaitForPreviousFrame(commandQueue);

	PIXEndEvent(commandList);
	PIXEndEvent(commandQueue);
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
	mScreenViewport.Width = static_cast<float>(mViewportWidth);
	mScreenViewport.Height = static_cast<float>(mViewportHeight);
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

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(1, slotRootParameters,
		0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> serializedRootSignature;
	ComPtr<ID3DBlob> error;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
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
void Renderer::CreatePSO(ID3D12Device* device, const std::string& psoName,
	const std::string& rootSignatureName, const std::string& shaderName)
{
	PipelineStateObject pso = nullptr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = mDepthStencil->GetDepthStencilBufferFormat();
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.NumRenderTargets = 1;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.pRootSignature = mRootSignatures[rootSignatureName].Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mShaders[shaderName + "VS"]->GetShader());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mShaders[shaderName + "PS"]->GetShader());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RTVFormats[0] = mSwapChain->GetBackBufferFormat();
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
	
	mPSOs.insert({ psoName, pso });
}