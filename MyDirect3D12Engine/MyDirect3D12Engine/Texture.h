#pragma once
#include <DDSTextureLoader.h>
#include "Stdafx.h"
#include "Utility.h"

class Texture
{
public:
	Texture() = default;

	void CreateTexture(
		ID3D12Device* device, 
		ID3D12GraphicsCommandList* commandList,
		const char* textureName, 
		const wchar_t* textureFilename);

	ID3D12Resource* GetTextureResource();

	static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
private:
	std::string mTextureName = "";
	std::wstring mTextureFilename = L"";

	Microsoft::WRL::ComPtr<ID3D12Resource> mTexture = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mTextureUpload = nullptr;
};