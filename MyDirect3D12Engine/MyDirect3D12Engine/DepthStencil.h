#pragma once
#include "Utility.h"

class DepthStencil
{
public:
	DepthStencil() = default;

	void CreateDepthStencilBuffer(ID3D12Device* device, 
		ID3D12GraphicsCommandList* commandList,
		UINT width, UINT height, 
		bool _4xMsaaState, UINT _4xMsaaQuality);

	void ResetDepthStencilBuffer();

	ID3D12Resource* GetDepthStencilBuffer();
	DXGI_FORMAT GetDepthStencilBufferFormat();
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer = nullptr;
	DXGI_FORMAT mDepthStencilBufferFormat = DXGI_FORMAT_R24G8_TYPELESS;
};