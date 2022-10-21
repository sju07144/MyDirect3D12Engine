#pragma once
#include "Utility.h"

template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT instanceCount, bool isConstantBuffer)
		: mIsConstantBuffer(isConstantBuffer)
	{
		mElementByteSize = sizeof(T);

		if (mIsConstantBuffer)
			mElementByteSize = D3D12Utility::CalculateConstantBufferSize(sizeof(T));

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(mElementByteSize * instanceCount)),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(mUploadBuffer.GetAddressOf())));

		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
	}
	~UploadBuffer()
	{
		if (mMappedData != nullptr)
			mUploadBuffer->Unmap(0, nullptr);

		mMappedData = nullptr;
	}

	ID3D12Resource* GetUploadBuffer()
	{
		return mUploadBuffer.Get();
	}
	UINT GetElementByteSize()
	{
		return mElementByteSize;
	}

	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex * mElementByteSize], &data, mElementByteSize);
	}
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer = nullptr;

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;

	BYTE* mMappedData = nullptr;
};
