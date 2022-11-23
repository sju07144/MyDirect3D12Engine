#pragma once
#include "../../Core/includes/Command.h"
#include "../../Core/includes/Stdafx.h"
#include "../../Core/includes/UploadBuffer.h"

class FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT objectCount, UINT sceneCount, 
		UINT materialCount, UINT instanceCount);
	FrameResource(const FrameResource& rhs) = default;
	FrameResource& operator=(const FrameResource& rhs) = default;
	~FrameResource() = default;

	UploadBuffer<ObjectConstant>* GetObjectConstantBuffers();
	UploadBuffer<SceneConstant>* GetSceneConstantBuffers();
	UploadBuffer<MaterialData>* GetMaterialBuffers();
	UploadBuffer<InstanceData>* GetInstanceBuffers();

	ID3D12CommandAllocator* GetDirectCommandAllocator();
	ID3D12GraphicsCommandList* GetDirectCommandList();
	ID3D12CommandAllocator* GetBundleAllocator();
	ID3D12GraphicsCommandList* GetBundle();

	void SetFenceValue(UINT64 fenceValue);
	UINT64 GetFenceValue();

	void ResetBundle();
private:
	std::unique_ptr<UploadBuffer<ObjectConstant>> mObjectCBs = nullptr;
	std::unique_ptr<UploadBuffer<SceneConstant>> mSceneCBs = nullptr;
	std::unique_ptr<UploadBuffer<MaterialData>> mMaterialBuffers = nullptr;
	std::unique_ptr<UploadBuffer<InstanceData>> mInstanceBuffers = nullptr;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCommandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mDirectCommandList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mBundleAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mBundle = nullptr;

	UINT64 mFenceValue = 0;
};