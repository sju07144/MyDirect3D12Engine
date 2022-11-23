#include "SobelFilter.h"

SobelFilter::SobelFilter(
	ID3D12Device* device, 
	UINT width, UINT height, 
	DXGI_FORMAT format)
	: mWidth(width), mHeight(height),
	mFormat(format)
{
	BuildResources(device);
}

void SobelFilter::BuildResources(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC textureDesc;
	textureDesc.Alignment = 0;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	textureDesc.Format = mFormat;
	textureDesc.Height = mHeight;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.MipLevels = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Width = mWidth;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mSobelMap)));
}

void SobelFilter::BuildDescriptors(
	ID3D12Device* device, 
	CbvSrvUavDescriptor& descriptorBuilder, 
	UINT cbvSrvUavDescriptorSize)
{
	int currentDescriptorIndex = descriptorBuilder.GetCurrentDescriptorIndex();

	descriptorBuilder.CreateShaderResourceView(device, cbvSrvUavDescriptorSize, mFormat,
		D3D12_SRV_DIMENSION_TEXTURE2D, mSobelMap.Get());
	descriptorBuilder.CreateUnorderedAccessView(device, cbvSrvUavDescriptorSize, mFormat,
		D3D12_UAV_DIMENSION_TEXTURE2D, mSobelMap.Get(), nullptr);

	mSobelMapSrvDescriptorIndex = currentDescriptorIndex + 1;
	mSobelMapUavDescriptorIndex = currentDescriptorIndex + 2;
}

void SobelFilter::Execute(
	ID3D12GraphicsCommandList* commandList, 
	ID3D12RootSignature* rootSignature, 
	ID3D12PipelineState* sobelPSO, 
	ID3D12Resource* inputTexture,
	int inputDescriptorIndex,
	CbvSrvUavDescriptor& descriptorBuilder, 
	UINT descriptorSize)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBlurMapSrv = descriptorBuilder.GetStartGPUDescriptorHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSobelMapUav = descriptorBuilder.GetStartGPUDescriptorHandle();
	gpuBlurMapSrv.Offset(inputDescriptorIndex, descriptorSize);
	gpuSobelMapUav.Offset(mSobelMapUavDescriptorIndex, descriptorSize);

	commandList->SetComputeRootSignature(rootSignature);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inputTexture,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSobelMap.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	commandList->SetComputeRootDescriptorTable(1, gpuBlurMapSrv);
	commandList->SetComputeRootDescriptorTable(2, gpuSobelMapUav);

	commandList->SetPipelineState(sobelPSO);

	commandList->Dispatch(16, 16, 1);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mSobelMap.Get(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void SobelFilter::ResizeSobelMap(
	UINT newWidth, UINT newHeight, 
	ID3D12Device* device, 
	CbvSrvUavDescriptor& descriptorBuilder, 
	UINT cbvSrvUavDescriptorSize)
{
	if (mWidth != newWidth || mHeight != newHeight)
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResources(device);

		BuildDescriptors(device, descriptorBuilder, cbvSrvUavDescriptorSize);
	}
}

ID3D12Resource* SobelFilter::GetSobelMap()
{
	return mSobelMap.Get();
}
int SobelFilter::GetSobelMapSrvDescriptorIndex()
{
	return mSobelMapSrvDescriptorIndex;
}
