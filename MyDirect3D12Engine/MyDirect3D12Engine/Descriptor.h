#pragma once
#include "Utility.h"

class Descriptor
{
public:
	Descriptor(ResourceDimension resourceDimension);
	virtual ~Descriptor() = default;

	void SetDescriptorSizes(ID3D12Device* device);

	UINT GetRtvDescriptorSize();
	UINT GetDsvDescriptorSize();
	UINT GetCbvSrvUavDescriptorSize();

	ID3D12DescriptorHeap* GetDescriptorHeap();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle();

	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) = 0;
	virtual ID3D12DescriptorHeap* GetDescriptorHeap() = 0;

	virtual void CreateDescriptor(ID3D12Device* device, DXGI_FORMAT viewFormat,
		ID3D12Resource* resource = nullptr, ID3D12Resource* counterResource = nullptr, UINT byteSize = 0) = 0;
	virtual CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() = 0;
	virtual CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() = 0;
protected:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDescriptorHeap = nullptr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mCpuDescriptorHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mGpuDescriptorHandle;
	UINT mDescriptorCount = 0;
	ResourceDimension mResourceDimension;


	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;
};

class RtvDescriptor : public Descriptor
{
public:
	RtvDescriptor(ResourceDimension resourceDimension);
	
	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) override;

	virtual void CreateDescriptor(ID3D12Device* device, DXGI_FORMAT viewFormat, 
		ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize) override;
};

class DsvDescriptor : public Descriptor
{
public:
	DsvDescriptor(ResourceDimension resourceDimension);

	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) override;

	virtual void CreateDescriptor(ID3D12Device* device, DXGI_FORMAT viewFormat, 
		ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize) override;
};

class CbvDescriptor : public Descriptor
{
public:
	CbvDescriptor(ResourceDimension resourceDimension);

	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) override;

	virtual void CreateDescriptor(ID3D12Device* device, DXGI_FORMAT viewFormat, 
		ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize) override;
};

class SrvDescriptor : public Descriptor
{
public:
	SrvDescriptor(ResourceDimension resourceDimension);

	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) override;

	virtual void CreateDescriptor(ID3D12Device* device, DXGI_FORMAT viewFormat, 
		ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize) override;
};

class UavDescriptor : public Descriptor
{
public:
	UavDescriptor(ResourceDimension resourceDimension);

	virtual void CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount) override;

	virtual void CreateDescriptor(ID3D12Device* device, DXGI_FORMAT viewFormat, 
		ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize) override;
};