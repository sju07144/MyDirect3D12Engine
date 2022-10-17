#pragma once
#include "Command.h"
#include "DepthStencil.h"
#include "Descriptor.h"
#include "Direct3d.h"
#include "Shader.h"
#include "SwapChain.h"
#include "Window.h"

using RootSignature = Microsoft::WRL::ComPtr<ID3D12RootSignature>;
using PipelineStateObject = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
using InputElement = D3D12_INPUT_ELEMENT_DESC;

// When the function or method has a command list parameter, executing command list is required.
// Ex) [Return Type] FunctionOrMethodName(..., ID3D12GraphicsCommandList*, ...)

class Renderer
{
public:
	Renderer() = default;
	Renderer(HINSTANCE hInstance);

	void Initialize();

	void Render();

private:
	int ProcessMessages();

	void ProcessInput();
	void UpdateData();
	void DrawScene();

	void EnableDebugLayer();
	void CheckMultiSamplingSupport(ID3D12Device* device, DXGI_FORMAT backBufferFormat);
	void ConfigureViewportAndScissorRect();
	void ConfigureInputElements();

	void CreateRootSignature(ID3D12Device* device, const std::string& rootSignatureName);
	void CreatePSO(ID3D12Device* device, const std::string& psoName);
private:
	std::unique_ptr<Window> mWindow;
	std::unique_ptr<BasicDirect3DComponent> mDirect3D;
	std::unique_ptr<Command> mCommand;
	std::unique_ptr<SwapChain> mSwapChain;
	std::unique_ptr<DepthStencil> mDepthStencil;

	std::unordered_map<std::string, std::unique_ptr<Shader>> mShaders;

	// std::vector<RootSignature> mRootSignatures; // default count is 1
	// std::vector<PipelineStateObject> mPSOs; // default count is 1
	// std::vector<InputLayout> mInputLayouts;

	std::unordered_map<std::string, RootSignature> mRootSignatures; // default count is 1
	std::unordered_map<std::string, PipelineStateObject> mPSOs; // default count is 1
	std::vector<InputElement> mInputLayout;

	std::array<std::unique_ptr<Descriptor>, 5> mDescriptors;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;
	UINT mViewportWidth;
	UINT mViewportHeight;

	bool m4xMsaaState = false;
	UINT m4xMsaaQuality = 0;
};