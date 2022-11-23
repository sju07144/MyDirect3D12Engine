#pragma once
#include "../../Core/includes/Stdafx.h"
#include "../../Core/includes/Descriptor.h"

class BlurFilter
{
public:
	BlurFilter(
		ID3D12Device* device,
		UINT width, UINT height,
		DXGI_FORMAT format);

	void BuildResources(ID3D12Device* device);

	void BuildDescriptors(
		ID3D12Device* device,
		CbvSrvUavDescriptor& descriptorBuilder, 
		UINT cbvSrvUavDescriptorSize);

	void Execute( 
		ID3D12GraphicsCommandList* commandList,
		ID3D12RootSignature* rootSignature,
		ID3D12PipelineState* horzBlurPSO,
		ID3D12PipelineState* vertBlurPSO,
		ID3D12Resource* inputTexture,
		CbvSrvUavDescriptor& descriptorBuilder,
		UINT descriptorSize,
		int blurCount);

	void ResizeBlurMap(UINT newWidth, UINT newHeight,
		ID3D12Device* device,
		CbvSrvUavDescriptor& descriptorBuilder,
		UINT cbvSrvUavDescriptorSize);

	ID3D12Resource* GetBlurMap();
	int GetBlurMapDescriptorIndex();

	std::vector<float> CalculateGaussWeights(float sigma);
private:
	static const int maxBlurRadius;

	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap0 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap1 = nullptr;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;

	int mBlurMap0SrvDescriptorIndex = -1;
	int mBlurMap0UavDescriptorIndex = -1;
	int mBlurMap1SrvDescriptorIndex = -1;
	int mBlurMap1UavDescriptorIndex = -1;
};