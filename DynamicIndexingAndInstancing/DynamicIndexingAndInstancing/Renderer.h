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

using RootSignature = Microsoft::WRL::ComPtr<ID3D12RootSignature>;
using PipelineStateObject = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
using InputElement = D3D12_INPUT_ELEMENT_DESC;

// When the function or method has a command list parameter, executing command list is required.
// Ex) [Return Type] FunctionOrMethodName(..., ID3D12GraphicsCommandList*, ...)

struct EnumHash
{
	template <typename T>
	std::size_t operator()(T t) const
	{
		return static_cast<std::size_t>(t);
	}
};

struct Light
{
	DirectX::XMFLOAT3 strength;
	float falloffStart; // point/spot light only
	DirectX::XMFLOAT3 direction; // directional/spot light only
	float falloffEnd; // point/spot light only
	DirectX::XMFLOAT3 position; // point light only
	float spotPower; // spot light only

	static constexpr int maxNumLights = 16;
};

struct Material
{
	std::string name;
	DirectX::XMFLOAT4 diffuseAlbedo;
	DirectX::XMFLOAT3 fresnelR0;
	float roughness;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Instancing,
	Sky,
	Count
};

struct ObjectConstant
{
	DirectX::XMFLOAT4X4 world;
	UINT materialIndex;
};

struct SceneConstant
{
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 proj;

	DirectX::XMFLOAT3 cameraPosition;
	float pad0;

	DirectX::XMFLOAT4 ambientLight;
	std::array<Light, Light::maxNumLights> lights;
};

struct MaterialData
{
	DirectX::XMFLOAT4 diffuseAlbedo;
	DirectX::XMFLOAT3 fresnelR0;
	float roughness;
};

struct InstanceData
{
	DirectX::XMFLOAT4X4 world;
	UINT materialIndex;
};


struct RenderItem
{
	Mesh mesh;
	DirectX::XMFLOAT4X4 world;
	UINT objectCBIndex = -1;
	UINT materialCBIndex = -1;
	UINT diffuseMapIndex = -1;
	UINT instanceCount = 0;
	std::vector<InstanceData> instanceDatas;
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

	void CreateRootSignature(ID3D12Device* device, const std::string& rootSignatureName);
	void CreateDefaultPSO(ID3D12Device* device, const std::string& psoName, 
		const std::string& rootSignatureName, const std::string& shaderName);
	void CreateSkyboxPSO(ID3D12Device* device, const std::string& psoName,
		const std::string& rootSignatureName, const std::string& shaderName);

	void LoadTextures();
	void BuildMaterials();

	void BuildRenderItems();
	void DrawRenderItems(RenderLayer renderLayer, ID3D12GraphicsCommandList* commandList);
private:
	// Window size variables.
	UINT mWindowWidth;
	UINT mWindowHeight;

	// Window handle and instance.
	HWND mhWnd = nullptr;
	HINSTANCE mhInstance = nullptr;

	static Renderer* renderer;

	BasicDirect3DComponent mDirect3D;
	Command mCommandObject;
	SwapChain mSwapChain;
	DepthStencil mDepthStencil;

	RtvDescriptor mRtvDescriptor;
	DsvDescriptor mDsvDescriptor;
	CbvSrvUavDescriptor mCbvSrvUavDescriptor;

	std::unordered_map<std::string, Shader> mShaders;

	std::unordered_map<std::string, RootSignature> mRootSignatures; // default count is 1
	std::unordered_map<std::string, PipelineStateObject> mPSOs; // default count is 1

	std::vector<InputElement> mInputLayout;

	Camera mCamera;
	
	Timer mTimer;

	std::unordered_map<std::string, Mesh> mMeshes;
	std::unordered_map<std::string, Texture> mTextures;
	std::unordered_map<std::string, Material> mMaterials;

	std::unique_ptr<UploadBuffer<ObjectConstant>> mObjectCBs = nullptr;
	std::unique_ptr<UploadBuffer<SceneConstant>> mSceneCBs = nullptr;
	std::unique_ptr<UploadBuffer<MaterialData>> mMaterialBuffers = nullptr;
	std::unique_ptr<UploadBuffer<InstanceData>> mInstanceBuffers = nullptr;

	std::vector<RenderItem> mOpaqueRenderItems;
	std::vector<RenderItem> mInstancingRenderItems;
	std::vector<RenderItem> mSkyRenderItems;
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