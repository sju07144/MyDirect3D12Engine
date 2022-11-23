#pragma once
#include <Windows.h>
#include <Windowsx.h>
#include <d3dx12.h>
#include <comdef.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <stb_image.h>
#include <array>
#include <cassert>
#include <codecvt>
#include <cstdio>
#include <locale>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using CommandAllocator = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;
using CommandQueue = Microsoft::WRL::ComPtr<ID3D12CommandQueue>;
using GraphicsCommandList = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>;
using Bundle = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>;

using RootSignature = Microsoft::WRL::ComPtr<ID3D12RootSignature>;
using PipelineStateObject = Microsoft::WRL::ComPtr<ID3D12PipelineState>;
using InputElement = D3D12_INPUT_ELEMENT_DESC;

// When the function or method has a command list parameter, executing command list is required.
// Ex) [Return Type] FunctionOrMethodName(..., ID3D12GraphicsCommandList*, ...)

struct EnumHash
{
	template <typename T>
	std::size_t operator()(T t) const
	{
		return static_cast<std::size_t>(t);
	}
};

struct Light
{
	DirectX::XMFLOAT3 strength;
	float falloffStart; // point/spot light only
	DirectX::XMFLOAT3 direction; // directional/spot light only
	float falloffEnd; // point/spot light only
	DirectX::XMFLOAT3 position; // point light only
	float spotPower; // spot light only

	static constexpr int maxNumLights = 16;
};

struct Material
{
	std::string name;
	DirectX::XMFLOAT4 diffuseAlbedo;
	DirectX::XMFLOAT3 fresnelR0;
	float roughness;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Instancing,
	Sky,
	Composite,
	Count
};

struct ObjectConstant
{
	DirectX::XMFLOAT4X4 world;
	UINT materialIndex;
};

struct SceneConstant
{
	DirectX::XMFLOAT4X4 view;
	DirectX::XMFLOAT4X4 proj;

	DirectX::XMFLOAT3 cameraPosition;
	float pad0;

	DirectX::XMFLOAT4 ambientLight;
	std::array<Light, Light::maxNumLights> lights;
};

struct MaterialData
{
	DirectX::XMFLOAT4 diffuseAlbedo;
	DirectX::XMFLOAT3 fresnelR0;
	float roughness;
};

struct InstanceData
{
	DirectX::XMFLOAT4X4 world;
	UINT materialIndex;
};