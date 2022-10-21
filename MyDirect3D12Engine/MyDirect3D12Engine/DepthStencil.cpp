#include "DepthStencil.h"

void DepthStencil::CreateDepthStencilBuffer(ID3D12Device* device,
	UINT width, UINT height,
	bool _4xMsaaState, UINT _4xMsaaQuality)
{
	D3D12_RESOURCE_DESC depthStencilBufferDesc;
	depthStencilBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilBufferDesc.Alignment = 0;
	depthStencilBufferDesc.DepthOrArraySize = 1;
	depthStencilBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	depthStencilBufferDesc.Format = mDepthStencilBufferFormat;
	depthStencilBufferDesc.Width = width;
	depthStencilBufferDesc.Height = height;
	depthStencilBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilBufferDesc.MipLevels = 0;
	depthStencilBufferDesc.SampleDesc.Count = 1;
	depthStencilBufferDesc.SampleDesc.Quality = 0;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilBufferFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilBufferDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optClear,
		IID_PPV_ARGS(&mDepthStencilBuffer)));
}

void DepthStencil::ResetDepthStencilBuffer()
{
	mDepthStencilBuffer.Reset();
}

ID3D12Resource* DepthStencil::GetDepthStencilBuffer()
{
	return mDepthStencilBuffer.Get();
}
DXGI_FORMAT DepthStencil::GetDepthStencilBufferFormat()
{
	return mDepthStencilBufferFormat;
}
