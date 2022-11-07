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
	mViewportWidth(800), mViewportHeight(600)
{
	renderer = this;

	float aspectRatio = static_cast<float>(mViewportWidth) / mViewportHeight;

	mCamera.LookAt(
		0.0f, 0.0f, -4.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f);

	mCamera.SetLens(0.25f * XM_PI, aspectRatio, 1.0f, 1000.0f);
}
Renderer::~Renderer()
{
	if (mDirect3D.GetDevice() != nullptr)
		mDirect3D.WaitForPreviousFrame(mCommandObject.GetCommandQueue());

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
	mDirect3D.CreateBasicDirect3DComponent();

	// Cache the objects
	auto device = mDirect3D.GetDevice();
	auto backBufferFormat = mSwapChain.GetBackBufferFormat();

	CheckMultiSamplingSupport(device, backBufferFormat);

	// Create command objects
	mCommandObject.CreateCommandAllocator(device);
	mCommandObject.CreateCommandQueue(device);
	mCommandObject.CreateGraphicsCommandList(device);

	auto commandList = mCommandObject.GetCommandList();
	auto commandQueue = mCommandObject.GetCommandQueue();

	// Create swap chain and back buffer
	mSwapChain.CreateSwapChain(mDirect3D.GetFactory(),
		commandQueue, mhWnd,
		mViewportWidth, mViewportHeight);

	// Create depth stencil buffer
	mDepthStencil.CreateDepthStencilBuffer(device,
		mViewportWidth, mViewportHeight, 
		m4xMsaaState, m4xMsaaQuality);

	// Create render target view
	auto backBufferCount = mSwapChain.GetBackBufferCount();
	mRtvDescriptor.CreateDescriptorHeap(device, backBufferCount);
	for (UINT i = 0; i < backBufferCount; i++)
		mRtvDescriptor.CreateRenderTargetView(device, mDirect3D.GetRtvDescriptorSize(), mSwapChain.GetBackBuffer(i));

	// Create depth stencil view
	mDsvDescriptor.CreateDescriptorHeap(device, 1);
	mDsvDescriptor.CreateDepthStencilView(device, mDirect3D.GetDsvDescriptorSize(),
		mDepthStencil.GetDepthStencilBufferFormat(), D3D12_DSV_DIMENSION_TEXTURE2D, 
		mDepthStencil.GetDepthStencilBuffer());

	// Create meshes
	BasicGeometryGenerator geoGenerator;
	auto box = geoGenerator.CreateBox(2.0f, 2.0f, 2.0f);
	box.ConfigureMesh(device, commandList);
	mMeshes.insert({ "box", std::move(box) });

	auto grid = geoGenerator.CreateGrid(10.0f, 20.0f, 10, 20);
	grid.ConfigureMesh(device, commandList);
	mMeshes.insert({ "grid", std::move(grid) });

	auto sphere = geoGenerator.CreateSphere(0.5f, 20, 20);
	sphere.ConfigureMesh(device, commandList);
	mMeshes.insert({ "sphere", std::move(sphere) });

	// Initialize constant buffer
	mObjectCBs = std::make_unique<UploadBuffer<ObjectConstant>>(device, 4, true);
	mSceneCBs = std::make_unique<UploadBuffer<SceneConstant>>(device, 1, true);
	mMaterialBuffers = std::make_unique<UploadBuffer<MaterialData>>(device, 4, false);
	mInstanceBuffers = std::make_unique<UploadBuffer<InstanceData>>(device, 100, false);

	// Create descriptor heap
	mCbvSrvUavDescriptor.CreateDescriptorHeap(device, 4);

	// Load Textures
	LoadTextures();

	// Build materials
	BuildMaterials();

	// Create descriptor heap and shader resource view
	auto woodTexture = mTextures["wood"].GetTextureResource();
	mCbvSrvUavDescriptor.CreateShaderResourceView(device, mDirect3D.GetCbvSrvUavDescriptorSize(),
		woodTexture->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, woodTexture);
	auto trinketTexture = mTextures["trinket"].GetTextureResource();
	mCbvSrvUavDescriptor.CreateShaderResourceView(device, mDirect3D.GetCbvSrvUavDescriptorSize(),
		trinketTexture->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, trinketTexture);
	auto aquaTexture = mTextures["aqua"].GetTextureResource();
	mCbvSrvUavDescriptor.CreateShaderResourceView(device, mDirect3D.GetCbvSrvUavDescriptorSize(),
		aquaTexture->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURE2D, aquaTexture);

	auto skyboxTexture = mTextures["sky"].GetTextureResource();
	mCbvSrvUavDescriptor.CreateShaderResourceView(device, mDirect3D.GetCbvSrvUavDescriptorSize(),
		skyboxTexture->GetDesc().Format, D3D12_SRV_DIMENSION_TEXTURECUBE, skyboxTexture);

	// Create vertex and pixel shader
	Shader vertexShader;
	Shader pixelShader; 
	vertexShader.CompileShader(L"../../Shaders/dynamicIndexing.hlsl", nullptr, "VSMain", "vs_5_1");
	pixelShader.CompileShader(L"../../Shaders/dynamicIndexing.hlsl", nullptr, "PSMain", "ps_5_1");
	mShaders.insert({ "opaqueVS", std::move(vertexShader) });
	mShaders.insert({ "opaquePS", std::move(pixelShader) });

	Shader instancingVertexShader;
	Shader instancingPixelShader;
	instancingVertexShader.CompileShader(L"../../Shaders/instancing.hlsl", nullptr, "VSMain", "vs_5_1");
	instancingPixelShader.CompileShader(L"../../Shaders/instancing.hlsl", nullptr, "PSMain", "ps_5_1");
	mShaders.insert({ "instancingVS", std::move(instancingVertexShader) });
	mShaders.insert({ "instancingPS", std::move(instancingPixelShader) });

	Shader skyVertexShader;
	Shader skyPixelShader;
	skyVertexShader.CompileShader(L"../../Shaders/sky.hlsl", nullptr, "VSMain", "vs_5_1");
	skyPixelShader.CompileShader(L"../../Shaders/sky.hlsl", nullptr, "PSMain", "ps_5_1");
	mShaders.insert({ "skyVS", std::move(skyVertexShader) });
	mShaders.insert({ "skyPS", std::move(skyPixelShader) });

	ConfigureInputElements();
	CreateRootSignature(device, "default");
	CreateDefaultPSO(device, "opaque", "default", "opaque");
	CreateDefaultPSO(device, "instancing", "default", "instancing");
	CreateSkyboxPSO(device, "sky", "default", "sky");

	BuildRenderItems();

	ExecuteCommandLists(commandList, commandQueue);

	mDirect3D.WaitForPreviousFrame(commandQueue);
}

int Renderer::RenderLoop()
{
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			mTimer.Tick();

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
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;
	case WM_SIZE:
		mWindowWidth = LOWORD(lParam);
		mWindowHeight = HIWORD(lParam);
		mViewportWidth = LOWORD(lParam);
		mViewportHeight = HIWORD(lParam);

		if (mDirect3D.GetDevice())
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
		mTimer.Stop();
		return 0;

	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
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
	auto device = mDirect3D.GetDevice();
	auto commandAllocator = mCommandObject.GetCommandAllocator();
	auto commandList = mCommandObject.GetCommandList();
	auto commandQueue = mCommandObject.GetCommandQueue();

	mDirect3D.WaitForPreviousFrame(commandQueue);

	// Resize back buffer
	mSwapChain.ResizeBackBuffers(mWindowWidth, mWindowHeight);
	
	mRtvDescriptor.ResetDescriptorHeap();
	for (UINT i = 0; i < mSwapChain.GetBackBufferCount(); i++)
		mRtvDescriptor.CreateRenderTargetView(device, mDirect3D.GetRtvDescriptorSize(), mSwapChain.GetBackBuffer(i));

	// Resize depth/stencil buffer
	mDepthStencil.ResetDepthStencilBuffer();
	mDepthStencil.CreateDepthStencilBuffer(device, mWindowWidth, mWindowHeight, m4xMsaaState, m4xMsaaQuality);

	mDsvDescriptor.ResetDescriptorHeap();
	mDsvDescriptor.CreateDepthStencilView(device, mDirect3D.GetDsvDescriptorSize(),
		mDepthStencil.GetDepthStencilBufferFormat(), D3D12_DSV_DIMENSION_TEXTURE2D, mDepthStencil.GetDepthStencilBuffer());

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

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void Renderer::ProcessKeyboardInput()
{
	const float dt = mTimer.DeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(10.0f * dt);
	
	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f * dt);
}
void Renderer::UpdateData()
{
	UpdateObjectConstants();
	UpdateSceneConstants();
	UpdateMaterialDatas();
	UpdateInstanceDatas();
}
void Renderer::DrawScene()
{
	auto device = mDirect3D.GetDevice();
	auto commandAllocator = mCommandObject.GetCommandAllocator();
	auto commandList = mCommandObject.GetCommandList();
	auto commandQueue = mCommandObject.GetCommandQueue();

	auto currentBackBuffer = mSwapChain.GetCurrentBackBuffer();

	ThrowIfFailed(commandAllocator->Reset());

	ThrowIfFailed(commandList->Reset(commandAllocator, mPSOs["opaque"].Get()));

	commandList->RSSetScissorRects(1, &mScissorRect);
	commandList->RSSetViewports(1, &mScreenViewport);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer,
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	D3D12_CPU_DESCRIPTOR_HANDLE currentRenderTargetView =
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvDescriptor.GetStartCPUDescriptorHandle(), 
		mSwapChain.GetCurrentBackBufferIndex(), mDirect3D.GetRtvDescriptorSize());
	commandList->ClearRenderTargetView(currentRenderTargetView, Colors::Black, 0, nullptr);
	commandList->ClearDepthStencilView(mDsvDescriptor.GetStartCPUDescriptorHandle(), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	commandList->OMSetRenderTargets(1, &currentRenderTargetView, true, &mDsvDescriptor.GetStartCPUDescriptorHandle());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvSrvUavDescriptor.GetDescriptorHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootSignature(mRootSignatures["default"].Get());

	auto sceneCBAddress = mSceneCBs->GetUploadBuffer()->GetGPUVirtualAddress();
	commandList->SetGraphicsRootConstantBufferView(1, sceneCBAddress);

	auto materialBufferAddress = mMaterialBuffers->GetUploadBuffer()->GetGPUVirtualAddress();
	commandList->SetGraphicsRootShaderResourceView(2, materialBufferAddress);

	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvUavDescriptor(mCbvSrvUavDescriptor.GetStartGPUDescriptorHandle());
	cbvSrvUavDescriptor.Offset(3, mDirect3D.GetCbvSrvUavDescriptorSize());
	commandList->SetGraphicsRootDescriptorTable(5, cbvSrvUavDescriptor);

	commandList->SetPipelineState(mPSOs["sky"].Get());
	DrawRenderItems(RenderLayer::Sky, commandList);

	commandList->SetPipelineState(mPSOs["opaque"].Get());
	DrawRenderItems(RenderLayer::Opaque, commandList);

	commandList->SetPipelineState(mPSOs["instancing"].Get());
	DrawRenderItems(RenderLayer::Instancing, commandList);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(currentBackBuffer,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ExecuteCommandLists(commandList, commandQueue);

	ThrowIfFailed(mSwapChain.GetSwapChain()->Present(0, 0));
	mSwapChain.SwitchBackBuffer();

	mDirect3D.WaitForPreviousFrame(commandQueue);
}

void Renderer::UpdateObjectConstants()
{
	ObjectConstant objectConstant;
	UINT elementIndex = 0;

	for (const auto& renderItems : mAllRenderItems)
	{
		for (const auto& renderItem: renderItems.second)
		{
			if (renderItem.instanceCount == 1)
			{
				XMMATRIX world = XMLoadFloat4x4(&renderItem.world);
				XMStoreFloat4x4(&objectConstant.world, XMMatrixTranspose(world));

				objectConstant.materialIndex = renderItem.materialCBIndex;

				mObjectCBs->CopyData(elementIndex, objectConstant);
				elementIndex++;
			}
		}
	}
}
void Renderer::UpdateSceneConstants()
{
	SceneConstant sceneConstant;

	XMFLOAT4X4 view = mCamera.GetView();
	XMFLOAT4X4 proj = mCamera.GetProj();

	XMMATRIX v = XMLoadFloat4x4(&view);
	XMMATRIX p = XMLoadFloat4x4(&proj);

	XMStoreFloat4x4(&sceneConstant.view, XMMatrixTranspose(v));
	XMStoreFloat4x4(&sceneConstant.proj, XMMatrixTranspose(p));

	sceneConstant.cameraPosition = mCamera.GetPosition();

	sceneConstant.ambientLight = XMFLOAT4{ 0.25f, 0.15f, 0.35f, 1.0f };
	sceneConstant.lights[0].direction = XMFLOAT3{ 0.0f, -1.0f, -1.0f };
	sceneConstant.lights[0].strength = XMFLOAT3{ 1.0f, 0.8f, 0.9f };

	mSceneCBs->CopyData(0, sceneConstant);
}
void Renderer::UpdateMaterialDatas()
{
	MaterialData materialData;
	UINT elementIndex = 0;

	for (const auto& material : mMaterials)
	{
		auto m = material.second;

		materialData.diffuseAlbedo = m.diffuseAlbedo;
		materialData.fresnelR0 = m.fresnelR0;
		materialData.roughness = m.roughness;

		mMaterialBuffers->CopyData(elementIndex, materialData);
		elementIndex++;
	}
}
void Renderer::UpdateInstanceDatas()
{
	InstanceData instanceData;
	UINT elementIndex = 0;

	for (const auto& renderItems : mAllRenderItems)
	{
		for (const auto& renderItem : renderItems.second)
		{
			if (renderItem.instanceCount > 1)
			{
				for (UINT i = 0; i < (UINT)renderItem.instanceDatas.size(); i++)
				{
					XMMATRIX world = XMLoadFloat4x4(&renderItem.instanceDatas[i].world);
					XMStoreFloat4x4(&instanceData.world, XMMatrixTranspose(world));

					instanceData.materialIndex = renderItem.instanceDatas[i].materialIndex;

					mInstanceBuffers->CopyData(elementIndex, instanceData);
					elementIndex++;
				}
			}
		}
	}
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

	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameters[6];
	slotRootParameters[0].InitAsConstantBufferView(0);
	slotRootParameters[1].InitAsConstantBufferView(1);
	slotRootParameters[2].InitAsShaderResourceView(0, 1);
	slotRootParameters[3].InitAsShaderResourceView(1, 1);
	slotRootParameters[4].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameters[5].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	auto samplers = Texture::GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(6, slotRootParameters,
		(UINT)samplers.size(), samplers.data(), 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
void Renderer::CreateDefaultPSO(ID3D12Device* device, const std::string& psoName,
	const std::string& rootSignatureName, const std::string& shaderName)
{
	PipelineStateObject pso = nullptr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = mDepthStencil.GetDepthStencilBufferFormat();
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.NumRenderTargets = 1;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.pRootSignature = mRootSignatures[rootSignatureName].Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mShaders[shaderName + "VS"].GetShader());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mShaders[shaderName + "PS"].GetShader());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RTVFormats[0] = mSwapChain.GetBackBufferFormat();
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.SampleMask = UINT_MAX;
	
	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
	
	mPSOs.insert({ psoName, pso });
}
void Renderer::CreateSkyboxPSO(ID3D12Device* device, const std::string& psoName, 
	const std::string& rootSignatureName, const std::string& shaderName)
{
	PipelineStateObject pso = nullptr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	psoDesc.DSVFormat = mDepthStencil.GetDepthStencilBufferFormat();
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.NumRenderTargets = 1;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.pRootSignature = mRootSignatures[rootSignatureName].Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(mShaders[shaderName + "VS"].GetShader());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(mShaders[shaderName + "PS"].GetShader());

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	psoDesc.RTVFormats[0] = mSwapChain.GetBackBufferFormat();
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.SampleMask = UINT_MAX;

	ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

	mPSOs.insert({ psoName, pso });
}

void Renderer::LoadTextures()
{
	// cache the d3d12 object
	auto device = mDirect3D.GetDevice();
	auto commandList = mCommandObject.GetCommandList();

	Texture woodTexture;
	std::string texName = "wood";
	woodTexture.CreateTexture(device, commandList, texName.c_str(), L"../../Textures/wood.dds");
	mTextures.insert({ texName, std::move(woodTexture) });

	Texture trinketTexture;
	texName = "trinket";
	trinketTexture.CreateTexture(device, commandList, texName.c_str(), L"../../Textures/trinket.dds");
	mTextures.insert({ texName, std::move(trinketTexture) });

	Texture aquaTexture;
	texName = "aqua";
	aquaTexture.CreateTexture(device, commandList, texName.c_str(), L"../../Textures/aqua.dds");
	mTextures.insert({ texName, std::move(aquaTexture) });

	Texture skyTexture;
	texName = "sky";
	skyTexture.CreateTexture(device, commandList, texName.c_str(), L"../../Textures/yokohama2.dds");
	mTextures.insert({ texName, std::move(skyTexture) });
}
void Renderer::BuildMaterials()
{
	Material woodBox;
	woodBox.name = "woodBox";
	woodBox.diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	woodBox.fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	woodBox.roughness = 0.01f;

	mMaterials.insert({ woodBox.name, std::move(woodBox) });

	Material trinketBox;
	trinketBox.name = "trinketBox";
	trinketBox.diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	trinketBox.fresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	trinketBox.roughness = 0.5f;

	mMaterials.insert({ trinketBox.name, std::move(trinketBox) });

	Material aquaGrid;
	aquaGrid.name = "aquaGrid";
	aquaGrid.diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	aquaGrid.fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	aquaGrid.roughness = 0.1f;

	mMaterials.insert({ aquaGrid.name, std::move(aquaGrid) });
}

void Renderer::BuildRenderItems()
{
	RenderItem renderItem;

	renderItem.mesh = mMeshes["box"];
	XMMATRIX world = XMMatrixTranslation(3.0f, 2.0f, 0.0f);
	XMStoreFloat4x4(&renderItem.world, world);
	renderItem.objectCBIndex = 0;
	renderItem.diffuseMapIndex = 0;
	renderItem.materialCBIndex = 0;
	renderItem.instanceCount = 1;
	mOpaqueRenderItems.push_back(renderItem);

	renderItem.mesh = mMeshes["box"];
	world = XMMatrixTranslation(-3.0f, -2.0f, 0.0f);
	XMStoreFloat4x4(&renderItem.world, world);
	renderItem.objectCBIndex = 1;
	renderItem.diffuseMapIndex = 1;
	renderItem.materialCBIndex = 1;
	renderItem.instanceCount = 1;
	mOpaqueRenderItems.push_back(renderItem);

	renderItem.mesh = mMeshes["grid"];
	world = XMMatrixTranslation(0.0f, -7.0f, 0.0f);
	XMStoreFloat4x4(&renderItem.world, world);
	renderItem.objectCBIndex = 2;
	renderItem.diffuseMapIndex = 2;
	renderItem.materialCBIndex = 2;
	renderItem.instanceCount = 1;
	mOpaqueRenderItems.push_back(renderItem);

	mAllRenderItems.insert({ RenderLayer::Opaque, mOpaqueRenderItems });

	std::random_device randomDevice;
	std::mt19937 generator(randomDevice());

	std::uniform_real_distribution<float> worldDistribution(-20.0f, 20.0f);
	std::uniform_int_distribution<int> materialIndexDistribution(0, 2);
	
	renderItem.mesh = mMeshes["box"];
	world = XMMatrixIdentity();
	XMStoreFloat4x4(&renderItem.world, world);
	renderItem.objectCBIndex = -1;
	renderItem.diffuseMapIndex = 2;
	renderItem.materialCBIndex = -1;
	renderItem.instanceCount = 50;
	renderItem.instanceDatas.resize(renderItem.instanceCount);

	XMMATRIX scalingMatrix = XMMatrixScaling(0.3f, 0.3f, 0.3f);
	for (UINT i = 0; i < renderItem.instanceCount; i++)
	{
		world = XMMatrixTranslation(worldDistribution(generator), worldDistribution(generator), worldDistribution(generator));
		world = XMMatrixMultiply(scalingMatrix, world);
		XMStoreFloat4x4(&renderItem.instanceDatas[i].world, world);
		renderItem.instanceDatas[i].materialIndex = materialIndexDistribution(generator);
	}
	mInstancingRenderItems.push_back(renderItem);

	mAllRenderItems.insert({ RenderLayer::Instancing, mInstancingRenderItems });

	renderItem.mesh = mMeshes["box"];
	world = XMMatrixScaling(5000.0f, 5000.0f, 5000.0f);
	XMStoreFloat4x4(&renderItem.world, world);
	renderItem.objectCBIndex = 3;
	renderItem.diffuseMapIndex = 3;
	renderItem.materialCBIndex = 3;
	renderItem.instanceCount = 1;
	mSkyRenderItems.push_back(renderItem);

	mAllRenderItems.insert({ RenderLayer::Sky, mSkyRenderItems });
}
void Renderer::DrawRenderItems(RenderLayer renderLayer, ID3D12GraphicsCommandList* commandList)
{
	UINT objectCBbyteSize = D3D12Utility::CalculateConstantBufferSize(sizeof(ObjectConstant));
	UINT cbvSrvUavDescriptorSize = mDirect3D.GetCbvSrvUavDescriptorSize();

	auto renderItems = mAllRenderItems[renderLayer];

	for (auto& renderItem : renderItems)
	{
		auto vbv = renderItem.mesh.GetVertexBufferView();
		auto ibv = renderItem.mesh.GetIndexBufferView();
		commandList->IASetVertexBuffers(0, 1, &vbv);
		commandList->IASetIndexBuffer(&ibv);
		commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (renderItem.instanceCount == 1)
		{
			auto objectCBAddress = mObjectCBs->GetUploadBuffer()->GetGPUVirtualAddress();
			objectCBAddress += renderItem.objectCBIndex * objectCBbyteSize;
			commandList->SetGraphicsRootConstantBufferView(0, objectCBAddress);
		}
		else if (renderItem.instanceCount > 1)
		{
			auto instanceBufferAddress = mInstanceBuffers->GetUploadBuffer()->GetGPUVirtualAddress();
			commandList->SetGraphicsRootShaderResourceView(3, instanceBufferAddress);
		}

		auto materialBufferAddress = mMaterialBuffers->GetUploadBuffer()->GetGPUVirtualAddress();
		commandList->SetGraphicsRootShaderResourceView(2, materialBufferAddress);

		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvUavDescriptor(mCbvSrvUavDescriptor.GetStartGPUDescriptorHandle());
		cbvSrvUavDescriptor.Offset(renderItem.diffuseMapIndex, mDirect3D.GetCbvSrvUavDescriptorSize());
		commandList->SetGraphicsRootDescriptorTable(4, cbvSrvUavDescriptor);

		commandList->DrawIndexedInstanced(renderItem.mesh.GetIndexCount(), 
			renderItem.instanceCount, 0, 0, 0);
	}
}