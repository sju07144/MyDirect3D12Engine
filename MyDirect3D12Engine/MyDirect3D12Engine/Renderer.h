#pragma once
#include "BasicGeometryGenerator.h"
#include "Camera.h"
#include "Command.h"
#include "DepthStencil.h"
#include "Descriptor.h"
#include "Direct3d.h"
#include "Mesh.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Timer.h"
#include "UploadBuffer.h"

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

struct ObjectConstant
{
	DirectX::XMFLOAT4X4 world;
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 proj;
	DirectX::XMFLOAT4X4 worldViewProj;
};

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class Renderer
{
public:
	Renderer() = default;
	Renderer(HINSTANCE hInstance);
	~Renderer();

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

	void EnableDebugLayer();
	void CheckMultiSamplingSupport(ID3D12Device* device, DXGI_FORMAT backBufferFormat);
	void ConfigureViewportAndScissorRect();
	void ConfigureInputElements();

	void CreateRootSignature(ID3D12Device* device, const std::string& rootSignatureName);
	void CreatePSO(ID3D12Device* device, const std::string& psoName, 
		const std::string& rootSignatureName, const std::string& shaderName);

private:
	// Window size variables.
	UINT mWindowWidth;
	UINT mWindowHeight;

	// Window handle and instance.
	HWND mhWnd = nullptr;
	HINSTANCE mhInstance = nullptr;

	static Renderer* renderer;

	std::unique_ptr<BasicDirect3DComponent> mDirect3D;
	std::unique_ptr<Command> mCommand;
	std::unique_ptr<SwapChain> mSwapChain;
	std::unique_ptr<DepthStencil> mDepthStencil;

	std::unordered_map<DescriptorType, std::unique_ptr<Descriptor>, EnumHash> mDescriptors;
	std::unordered_map<std::string, std::unique_ptr<Shader>> mShaders;

	std::unordered_map<std::string, RootSignature> mRootSignatures; // default count is 1
	std::unordered_map<std::string, PipelineStateObject> mPSOs; // default count is 1
	std::vector<InputElement> mInputLayout;

	std::unique_ptr<Camera> mCamera;

	std::unique_ptr<Timer> mTimer;

	std::unordered_map<std::string, std::unique_ptr<Mesh>> mMeshes;

	std::unique_ptr<UploadBuffer<ObjectConstant>> mObjectCBs = nullptr;

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