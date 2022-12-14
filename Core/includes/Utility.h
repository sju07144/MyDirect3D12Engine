#pragma once
#include "Stdafx.h"

/* inline std::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
	HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
	HRESULT Error() const { return m_hr; }
private:
	const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release();

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
		throw HrException(hr);
} */

inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

	std::wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

struct Vertex
{
	Vertex() = default;
	Vertex(float px, float py, float pz, 
		float nx, float ny, float nz, 
		float u, float v)
		: position(DirectX::XMFLOAT3(px, py, pz)),
		normal(DirectX::XMFLOAT3(nx, ny, nz)),
		texCoord(DirectX::XMFLOAT2(u, v))
	{ }

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texCoord;
};

class D3D12Utility
{
public:
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* commandList,
		const void* initData,
		UINT byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static UINT CalculateConstantBufferSize(UINT size);

	static std::wstring ImageFormatToDDS(const std::wstring& filePath);
};

class MathUtility
{
public:
	static const float PI;
};