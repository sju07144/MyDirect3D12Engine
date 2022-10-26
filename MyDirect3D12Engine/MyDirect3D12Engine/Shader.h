#pragma once
#include "Stdafx.h"
#include "Utility.h"

class Shader
{
public:
	Shader() = default;

	void CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	ID3DBlob* GetShader();
private:
	Microsoft::WRL::ComPtr<ID3DBlob> mShader;
};