#pragma once
#pragma once
#include "../../Core/includes/Stdafx.h"
#include "../../Core/includes/Descriptor.h"

class SobelFilter
{
public:
	SobelFilter(
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
		ID3D12PipelineState* sobelPSO,
		ID3D12Resource* inputTexture,
		int inputDescriptorIndex,
		CbvSrvUavDescriptor& descriptorBuilder,
		UINT descriptorSize);

	void ResizeSobelMap(UINT newWidth, UINT newHeight,
		ID3D12Device* device,
		CbvSrvUavDescriptor& descriptorBuilder,
		UINT cbvSrvUavDescriptorSize);

	ID3D12Resource* GetSobelMap();
	int GetSobelMapSrvDescriptorIndex();
private:
	static const int maxBlurRadius;

	Microsoft::WRL::ComPtr<ID3D12Resource> mSobelMap = nullptr;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;

	int mSobelMapSrvDescriptorIndex = -1;
	int mSobelMapUavDescriptorIndex = -1;
};