#include "BlurFilter.h"

const int BlurFilter::maxBlurRadius = 5;

BlurFilter::BlurFilter(
	ID3D12Device* device,
	UINT width, UINT height, 
	DXGI_FORMAT format)
	: mWidth(width), mHeight(height), mFormat(format)
{
	BuildResources(device);
}

void BlurFilter::BuildResources(ID3D12Device* device)
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
		IID_PPV_ARGS(&mBlurMap0)));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mBlurMap1)));
}

void BlurFilter::BuildDescriptors(
	ID3D12Device* device,
	CbvSrvUavDescriptor& descriptorBuilder, 
	UINT cbvSrvUavDescriptorSize)
{
	int currentDescriptorIndex = descriptorBuilder.GetCurrentDescriptorIndex();

	descriptorBuilder.CreateShaderResourceView(device, cbvSrvUavDescriptorSize,
		mFormat, D3D12_SRV_DIMENSION_TEXTURE2D, mBlurMap0.Get());
	descriptorBuilder.CreateUnorderedAccessView(device, cbvSrvUavDescriptorSize,
		mFormat, D3D12_UAV_DIMENSION_TEXTURE2D, mBlurMap0.Get(), nullptr);

	descriptorBuilder.CreateShaderResourceView(device, cbvSrvUavDescriptorSize,
		mFormat, D3D12_SRV_DIMENSION_TEXTURE2D, mBlurMap1.Get());
	descriptorBuilder.CreateUnorderedAccessView(device, cbvSrvUavDescriptorSize,
		mFormat, D3D12_UAV_DIMENSION_TEXTURE2D, mBlurMap1.Get(), nullptr);

	mBlurMap0SrvDescriptorIndex = currentDescriptorIndex + 1;
	mBlurMap0UavDescriptorIndex = currentDescriptorIndex + 2;
	mBlurMap1SrvDescriptorIndex = currentDescriptorIndex + 3;
	mBlurMap1UavDescriptorIndex = currentDescriptorIndex + 4;
}

void BlurFilter::Execute(
	ID3D12GraphicsCommandList* commandList,
	ID3D12RootSignature* rootSignature,
	ID3D12PipelineState* horzBlurPSO,
	ID3D12PipelineState* vertBlurPSO,
	ID3D12Resource* inputTexture,
	CbvSrvUavDescriptor& descriptorBuilder,
	UINT descriptorSize,
	int blurCount)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBlurMap0Srv = descriptorBuilder.GetStartGPUDescriptorHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBlurMap0Uav = descriptorBuilder.GetStartGPUDescriptorHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBlurMap1Srv = descriptorBuilder.GetStartGPUDescriptorHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBlurMap1Uav = descriptorBuilder.GetStartGPUDescriptorHandle();
	gpuBlurMap0Srv.Offset(mBlurMap0SrvDescriptorIndex, descriptorSize);
	gpuBlurMap0Uav.Offset(mBlurMap0UavDescriptorIndex, descriptorSize);
	gpuBlurMap1Srv.Offset(mBlurMap1SrvDescriptorIndex, descriptorSize);
	gpuBlurMap1Uav.Offset(mBlurMap1UavDescriptorIndex, descriptorSize);

	auto weights = CalculateGaussWeights(2.5f);
	int blurRadius = static_cast<int>(weights.size()) / 2;

	commandList->SetComputeRootSignature(rootSignature);

	commandList->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
	commandList->SetComputeRoot32BitConstants(0, static_cast<UINT>(weights.size()),
		weights.data(), 1);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(inputTexture,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));

	commandList->CopyResource(mBlurMap0.Get(), inputTexture);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	for (int i = 0; i < blurCount; i++)
	{
		// horizontal blur pass
		commandList->SetPipelineState(horzBlurPSO);

		commandList->SetComputeRootDescriptorTable(1, gpuBlurMap0Srv);
		commandList->SetComputeRootDescriptorTable(2, gpuBlurMap1Uav);

		UINT numGroupsX = static_cast<UINT>(ceilf(static_cast<float>(mWidth) / 256.0f));
		commandList->Dispatch(numGroupsX, mHeight, 1);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		// vertical blur pass
		commandList->SetPipelineState(vertBlurPSO);

		commandList->SetComputeRootDescriptorTable(1, gpuBlurMap1Srv);
		commandList->SetComputeRootDescriptorTable(2, gpuBlurMap0Uav);

		UINT numGroupsY = static_cast<UINT>(ceilf(static_cast<float>(mHeight) / 256.0f));
		commandList->Dispatch(mWidth, numGroupsY, 1);

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
	}
}

void BlurFilter::ResizeBlurMap(UINT newWidth, UINT newHeight, 
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

ID3D12Resource* BlurFilter::GetBlurMap()
{
	return mBlurMap0.Get();
}
int BlurFilter::GetBlurMapDescriptorIndex()
{
	return mBlurMap0SrvDescriptorIndex;
}

std::vector<float> BlurFilter::CalculateGaussWeights(float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	// For example, for sigma = 3, the width of the bell curve is 
	int blurRadius = static_cast<int>(ceil(2.0f * sigma));

	assert(blurRadius <= maxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		float x = static_cast<float>(i);

		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < weights.size(); i++)
	{
		weights[i] /= weightSum;
	}

	return weights;
}