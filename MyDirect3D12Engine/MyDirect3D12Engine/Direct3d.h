#pragma once
#include "Utility.h"

class BasicDirect3DComponent
{
public:
	BasicDirect3DComponent() = default;

	void CreateBasicDirect3DComponent();

	void WaitForPreviousFrame(ID3D12CommandQueue* commandQueue);

	IDXGIFactory4* GetFactory();
	ID3D12Device* GetDevice();
	ID3D12Fence* GetFence();

private:
	void CreateFactory();
	void CreateDevice();
	void CreateFence();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

private:
	Microsoft::WRL::ComPtr<IDXGIFactory4> mFactory = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Device> mDevice = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence = nullptr;
	UINT64 mCurrentFenceValue = 0;
};