#pragma once
#include "Stdafx.h"
#include "Utility.h"

class DepthStencil
{
public:
	DepthStencil() = default;

	void CreateDepthStencilBuffer(ID3D12Device* device, 
		UINT width, UINT height, 
		bool _4xMsaaState, UINT _4xMsaaQuality);

	void ResetDepthStencilBuffer();

	ID3D12Resource* GetDepthStencilBuffer();
	DXGI_FORMAT GetDepthStencilBufferFormat();
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer = nullptr;
	DXGI_FORMAT mDepthStencilBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};