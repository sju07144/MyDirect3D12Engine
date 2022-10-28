#include "Descriptor.h"

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

void RtvDescriptor::CreateRenderTargetView(ID3D12Device* device, UINT descriptorSize, ID3D12Resource* resource)
{
	device->CreateRenderTargetView(resource, nullptr, mCpuDescriptorHandle);

	mCpuDescriptorHandle.Offset(1, descriptorSize);

	mDescriptorCount++;
}

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

void DsvDescriptor::CreateDepthStencilView(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat, 
	D3D12_DSV_DIMENSION viewDimension, ID3D12Resource* resource)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = viewFormat;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = viewDimension;

	switch (viewDimension)
	{
	case D3D12_DSV_DIMENSION_TEXTURE2D:
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

void CbvSrvUavDescriptor::CreateDescriptorHeap(ID3D12Device* device, UINT descriptorCount)
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc;
	cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvSrvUavHeapDesc.NodeMask = 0;
	cbvSrvUavHeapDesc.NumDescriptors = descriptorCount;
	cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ThrowIfFailed(device->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&mDescriptorHeap)));

	mCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	mGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void CbvSrvUavDescriptor::CreateConstantBufferView(ID3D12Device* device, UINT descriptorSize, UINT elementIndex,
	ID3D12Resource* resource, UINT byteSize)
{
	auto cbAddress = resource->GetGPUVirtualAddress() + elementIndex * byteSize;

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(cbAddress); // need to revise
	cbvDesc.SizeInBytes = byteSize;

	device->CreateConstantBufferView(&cbvDesc, mCpuDescriptorHandle);

	mCpuDescriptorHandle.Offset(1, descriptorSize);

	mDescriptorCount++;

	mCbvCount++;
}
void CbvSrvUavDescriptor::CreateShaderResourceView(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat, 
	D3D12_SRV_DIMENSION viewDimension, ID3D12Resource* resource)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = viewFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = viewDimension;

	switch (viewDimension)
	{
	case D3D12_SRV_DIMENSION_TEXTURE2D:
		if (resource)
			srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		else
			srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	default:
		throw std::runtime_error("Invalid resource dimension");
		break;
	}

	device->CreateShaderResourceView(resource, &srvDesc, mCpuDescriptorHandle);

	mCpuDescriptorHandle.Offset(1, descriptorSize);

	mDescriptorCount++;

	mSrvCount++;
}
void CbvSrvUavDescriptor::CreateUnorderedAccessView(ID3D12Device* device, UINT descriptorSize, DXGI_FORMAT viewFormat, 
	D3D12_UAV_DIMENSION viewDimension, ID3D12Resource* resource, ID3D12Resource* counterResource)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.ViewDimension = viewDimension;

	switch (viewDimension)
	{
	case D3D12_UAV_DIMENSION_TEXTURE2D:
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

	mUavCount++;
}