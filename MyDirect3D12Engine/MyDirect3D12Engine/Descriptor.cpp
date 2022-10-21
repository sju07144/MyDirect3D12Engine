#include "Descriptor.h"

Descriptor::Descriptor(ResourceDimension resourceDimension)
	: mResourceDimension(resourceDimension)
{ }

ID3D12DescriptorHeap* Descriptor::GetDescriptorHeap()
{
	return mDescriptorHeap.Get();
}
CD3DX12_CPU_DESCRIPTOR_HANDLE Descriptor::GetStartCPUDescriptorHandle()
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}
CD3DX12_GPU_DESCRIPTOR_HANDLE Descriptor::GetStartGPUDescriptorHandle()
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void Descriptor::ResetDescriptorHeap()
{
	mCpuDescriptorHandle = mDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	mGpuDescriptorHandle = mDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	mDescriptorCount = 0;
}

RtvDescriptor::RtvDescriptor(ResourceDimension resourceDimension)
	: Descriptor(resourceDimension)
{ }

void RtvDescriptor::CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	rtvHeapDesc.NumDescriptors = descriptorCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mDescriptorHeap.GetAddressOf())));

	mCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	mGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void RtvDescriptor::CreateDescriptor(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat,
	ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize)
{
	/* D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = viewFormat;

	switch (mResourceDimension)
	{
	case ResourceDimension::texture2D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		break;
	default:
		throw std::runtime_error("Invalid resource dimension");
		break;
	} */

	device->CreateRenderTargetView(resource, nullptr, mCpuDescriptorHandle);

	mCpuDescriptorHandle.Offset(1, descriptorSize);

	mDescriptorCount++;
}

DsvDescriptor::DsvDescriptor(ResourceDimension resourceDimension)
	: Descriptor(resourceDimension)
{ }

void DsvDescriptor::CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	dsvHeapDesc.NumDescriptors = descriptorCount;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDescriptorHeap)));

	mCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	mGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void DsvDescriptor::CreateDescriptor(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat,
	ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = viewFormat;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	switch (mResourceDimension)
	{
	case ResourceDimension::texture2D:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		break;
	default:
		throw std::runtime_error("Invalid resource dimension");
		break;
	}

	device->CreateDepthStencilView(resource, &dsvDesc, mCpuDescriptorHandle);

	mCpuDescriptorHandle.Offset(1, descriptorSize);

	mDescriptorCount++;
}

CbvDescriptor::CbvDescriptor(ResourceDimension resourceDimension)
	: Descriptor(resourceDimension)
{ }

void CbvDescriptor::CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	cbvHeapDesc.NumDescriptors = descriptorCount;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mDescriptorHeap)));

	mCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	mGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void CbvDescriptor::CreateDescriptor(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat,
	ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = resource->GetGPUVirtualAddress(); // need to revise
	cbvDesc.SizeInBytes = byteSize;

	device->CreateConstantBufferView(&cbvDesc, mCpuDescriptorHandle);

	mCpuDescriptorHandle.Offset(1, descriptorSize);

	mDescriptorCount++;
}

SrvDescriptor::SrvDescriptor(ResourceDimension resourceDimension)
	: Descriptor(resourceDimension)
{ }

void SrvDescriptor::CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NodeMask = 0;
	srvHeapDesc.NumDescriptors = descriptorCount;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mDescriptorHeap)));

	mCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	mGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void SrvDescriptor::CreateDescriptor(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat,
	ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = viewFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (mResourceDimension)
	{
	case ResourceDimension::texture2D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 0;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0;
		break;
	default:
		throw std::runtime_error("Invalid resource dimension");
		break;
	}

	device->CreateShaderResourceView(resource, &srvDesc, mCpuDescriptorHandle);

	mCpuDescriptorHandle.Offset(1, descriptorSize);

	mDescriptorCount++;
}

UavDescriptor::UavDescriptor(ResourceDimension resourceDimension)
	: Descriptor(resourceDimension)
{ }

void UavDescriptor::CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC uavHeapDesc;
	uavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	uavHeapDesc.NodeMask = 0;
	uavHeapDesc.NumDescriptors = descriptorCount;
	uavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ThrowIfFailed(device->CreateDescriptorHeap(&uavHeapDesc, IID_PPV_ARGS(&mDescriptorHeap)));

	mCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	mGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void UavDescriptor::CreateDescriptor(ID3D12Device* device, UINT descriptorSize,DXGI_FORMAT viewFormat,
	ID3D12Resource* resource, ID3D12Resource* counterResource, UINT byteSize)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;

	switch (mResourceDimension)
	{
	case ResourceDimension::texture2D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		uavDesc.Texture2D.PlaneSlice = 0;
		break;
	default:
		throw std::runtime_error("Invalid resource dimension");
		break;
	}

	device->CreateUnorderedAccessView(resource, counterResource, &uavDesc, mCpuDescriptorHandle);

	mCpuDescriptorHandle.Offset(1, descriptorSize);

	mDescriptorCount++;
}