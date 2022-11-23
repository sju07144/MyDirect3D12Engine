#pragma once
#include "../../Core/includes/BasicGeometryGenerator.h"
#include "../../Core/includes/Camera.h"
#include "../../Core/includes/Command.h"
#include "../../Core/includes/DepthStencil.h"
#include "../../Core/includes/Descriptor.h"
#include "../../Core/includes/Direct3d.h"
#include "../../Core/includes/Mesh.h"
#include "../../Core/includes/Model.h"
#include "../../Core/includes/Shader.h"
#include "../../Core/includes/Stdafx.h"
#include "../../Core/includes/SwapChain.h"
#include "../../Core/includes/Texture.h"
#include "../../Core/includes/Timer.h"
#include "../../Core/includes/UploadBuffer.h"
#include "../../Core/includes/Utility.h"
#include "BlurFilter.h"
#include "FrameResource.h"
#include "SobelFilter.h"

struct RenderItem
{
	Mesh mesh;
	DirectX::XMFLOAT4X4 world;
	UINT objectCBIndex = -1;
	UINT materialCBIndex = -1;
	UINT diffuseMapIndex = -1;
	UINT instanceCount = 0;
	std::vector<InstanceData> instanceDatas;

	UINT numFrameDirty = 3;
};

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class Renderer
{
public:
	Renderer() = default;
	Renderer(HINSTANCE hInstance);
	~Renderer();
	Renderer(const Renderer& rhs) = delete;
	Renderer operator=(const Renderer& rhs) = delete;

	void Initialize();

	int RenderLoop();

	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static Renderer* GetRendererPointer();

	static const UINT mFrameResourceCount;
private:
	bool InitializeWindow();

	void Resize();

	void ExecuteCommandLists(ID3D12GraphicsCommandList* commandList,
		ID3D12CommandQueue* commandQueue);

	void MouseDown(WPARAM btnState, int x, int y);
	void MouseUp(WPARAM btnState, int x, int y);
	void MouseMove(WPARAM btnState, int x, int y);

	void ProcessKeyboardInput();
	void UpdateData();
	void DrawScene();

	void UpdateObjectConstants();
	void UpdateSceneConstants();
	void UpdateMaterialDatas();
	void UpdateInstanceDatas();

	void EnableDebugLayer();
	void CheckMultiSamplingSupport(ID3D12Device* device, DXGI_FORMAT backBufferFormat);
	void ConfigureViewportAndScissorRect();
	void ConfigureInputElements();

	void CreateDirectCommandQueue(ID3D12Device* device);

	void CreateDefaultRootSignature(ID3D12Device* device);
	void CreatePostProcessRootSignature(ID3D12Device* device);
	void CreateDefaultPSO(ID3D12Device* device, const std::string& psoName, 
		const std::string& rootSignatureName, const std::string& shaderName);
	void CreateSkyboxPSO(ID3D12Device* device, const std::string& psoName,
		const std::string& rootSignatureName, const std::string& shaderName);
	void CreateBlurPSO(ID3D12Device* device);
	void CreateSobelPSO(ID3D12Device* device);

	void LoadTextures();
	void BuildMaterials();

	void BuildRenderItems();
	void DrawRenderItems(RenderLayer renderLayer, ID3D12GraphicsCommandList* commandList, 
		ID3D12PipelineState* pipelineState);
private:
	// Window size variables.
	UINT mWindowWidth;
	UINT mWindowHeight;

	// Window handle and instance.
	HWND mhWnd = nullptr;
	HINSTANCE mhInstance = nullptr;

	static Renderer* renderer;

	BasicDirect3DComponent mDirect3D;

	Command mInitializeCommandObject;
	CommandQueue mDirectCommandQueue;

	SwapChain mSwapChain;
	DepthStencil mDepthStencil;

	RtvDescriptor mRtvDescriptor;
	DsvDescriptor mDsvDescriptor;
	CbvSrvUavDescriptor mCbvSrvUavDescriptor;

	std::unordered_map<std::string, Shader> mShaders;

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrentFrameResource = nullptr;
	UINT mCurrentFrameResourceIndex = 0;

	std::unordered_map<std::string, RootSignature> mRootSignatures; // default count is 1
	std::unordered_map<std::string, PipelineStateObject> mPSOs; // default count is 1

	std::unique_ptr<BlurFilter> mBlurFilter = nullptr;
	std::unique_ptr<SobelFilter> mSobelFilter = nullptr;

	std::vector<InputElement> mInputLayout;

	Camera mCamera;
	
	Timer mTimer;

	Texture mRenderTexture;

	std::unordered_map<std::string, Mesh> mMeshes;
	std::unordered_map<std::string, Texture> mTextures;
	std::unordered_map<std::string, Material> mMaterials;

	std::vector<RenderItem> mOpaqueRenderItems;
	std::vector<RenderItem> mInstancingRenderItems;
	std::vector<RenderItem> mSkyRenderItems;
	std::vector<RenderItem> mCompositeRenderItems;
	std::unordered_map<RenderLayer, std::vector<RenderItem>> mAllRenderItems;

	POINT mLastMousePos = { 0, 0 };

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;
	UINT mViewportWidth;
	UINT mViewportHeight;

	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;

	bool mAppPaused = true;
	bool mMinimized = true;
	bool mMaximized = false;
	bool mResizing = false;
};