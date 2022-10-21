#include "Direct3d.h"

using namespace Microsoft::WRL;

void BasicDirect3DComponent::CreateBasicDirect3DComponent()
{
	CreateFactory();
	CreateDevice();
	SetDescriptorSize();
	CreateFence();

#ifdef DEBUG
	LogAdapters();
#endif
}

void BasicDirect3DComponent::WaitForPreviousFrame(ID3D12CommandQueue* commandQueue)
{
	mCurrentFenceValue++;
	ThrowIfFailed(commandQueue->Signal(mFence.Get(), mCurrentFenceValue));

	if (mFence->GetCompletedValue() < mCurrentFenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFenceValue, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void BasicDirect3DComponent::SetDescriptorSize()
{
	mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

IDXGIFactory4* BasicDirect3DComponent::GetFactory()
{
	return mFactory.Get();
}
ID3D12Device* BasicDirect3DComponent::GetDevice()
{
	return mDevice.Get();
}
ID3D12Fence* BasicDirect3DComponent::GetFence()
{
	return mFence.Get();
}

UINT BasicDirect3DComponent::GetRtvDescriptorSize()
{
	return mRtvDescriptorSize;
}
UINT BasicDirect3DComponent::GetDsvDescriptorSize()
{
	return mDsvDescriptorSize;
}
UINT BasicDirect3DComponent::GetCbvSrvUavDescriptorSize()
{
	return mCbvSrvUavDescriptorSize;
}

void BasicDirect3DComponent::CreateFactory()
{
	ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&mFactory)));
}
void BasicDirect3DComponent::CreateDevice()
{
	HRESULT hr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&mDevice));
	if (FAILED(hr))
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		mFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));
		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&mDevice)));
	}
}
void BasicDirect3DComponent::CreateFence()
{
	ThrowIfFailed(mDevice->CreateFence(mCurrentFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
}

void BasicDirect3DComponent::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter;
	std::vector<IDXGIAdapter*> adapterList;

	while (mFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC adapterDesc;
		adapter->GetDesc(&adapterDesc);
		
		std::wstring text = L"Adapter: ";
		text += adapterDesc.Description;
		text += L"\n";
		::OutputDebugString(text.c_str());

		adapterList.push_back(adapter);
		i++;
	}

	for (auto& adapter : adapterList)
	{
		LogAdapterOutputs(adapter);
		ReleaseCom(adapter);
	}
}
void BasicDirect3DComponent::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	UINT i = 0;
	IDXGIOutput* output;

	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC outputDesc;
		output->GetDesc(&outputDesc);

		std::wstring text = L"Output: ";
		text += outputDesc.DeviceName;
		text += L"\n";
		::OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, DXGI_FORMAT_R8G8B8A8_UNORM);

		ReleaseCom(output);

		i++;
	}
}
void BasicDirect3DComponent::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	output->GetDisplayModeList(format, flags, &count, nullptr);
	std::vector<DXGI_MODE_DESC> displayModeList;
	output->GetDisplayModeList(format, flags, &count, displayModeList.data());

	for (const auto& x : displayModeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}