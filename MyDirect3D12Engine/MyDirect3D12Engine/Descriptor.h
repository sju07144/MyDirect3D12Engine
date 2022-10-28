#pragma once
#include "Stdafx.h"
#include "Utility.h"

class Descriptor
{
public:
	Descriptor() = default;
	virtual ~Descriptor() = default;

	ID3D12DescriptorHeap* GetDescriptorHeap();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetStartCPUDescriptorHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetStartGPUDescriptorHandle();

	void ResetDescriptorHeap();

	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) = 0;

protected:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDescriptorHeap = nullptr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuDescriptorHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuDescriptorHandle;
	UINT mDescriptorCount = 0;
};

class RtvDescriptor : public Descriptor
{
public:
	RtvDescriptor() = default;
	
	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) override;

	void CreateRenderTargetView(ID3D12Device* device, UINT descriptorSize, ID3D12Resource* resource);
};

class DsvDescriptor : public Descriptor
{
public:
	DsvDescriptor() = default;

	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) override;

	void CreateDepthStencilView(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat,
		D3D12_DSV_DIMENSION viewDimension, ID3D12Resource* resource);
};

class CbvSrvUavDescriptor : public Descriptor
{
public:
	CbvSrvUavDescriptor() = default;

	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) override;

	void CreateConstantBufferView(ID3D12Device* device, UINT descriptorSize, UINT elementIndex,
		ID3D12Resource* resource, UINT byteSize);
	void CreateShaderResourceView(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat,
		D3D12_SRV_DIMENSION viewDimension, ID3D12Resource* resource);
	void CreateUnorderedAccessView(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat,
		D3D12_UAV_DIMENSION viewDimension, ID3D12Resource* resource, ID3D12Resource* counterResource);

private:
	UINT mCbvCount = 0;
	UINT mSrvCount = 0;
	UINT mUavCount = 0;
};