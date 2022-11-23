#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT objectCount, 
	UINT sceneCount, UINT materialCount, UINT instanceCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&mDirectCommandAllocator)));
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mDirectCommandList)));

	ThrowIfFailed(mDirectCommandList->Close());

	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE,
		IID_PPV_ARGS(&mBundleAllocator)));
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE,
		mBundleAllocator.Get(), nullptr, IID_PPV_ARGS(&mBundle)));

	ThrowIfFailed(mBundle->Close());

	mObjectCBs = std::make_unique<UploadBuffer<ObjectConstant>>(device, objectCount, true);
	mSceneCBs = std::make_unique<UploadBuffer<SceneConstant>>(device, sceneCount, true);
	mMaterialBuffers = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
	mInstanceBuffers = std::make_unique<UploadBuffer<InstanceData>>(device, instanceCount, false);
}

UploadBuffer<ObjectConstant>* FrameResource::GetObjectConstantBuffers()
{
	return mObjectCBs.get();
}
UploadBuffer<SceneConstant>* FrameResource::GetSceneConstantBuffers()
{
	return mSceneCBs.get();
}
UploadBuffer<MaterialData>* FrameResource::GetMaterialBuffers()
{
	return mMaterialBuffers.get();
}
UploadBuffer<InstanceData>* FrameResource::GetInstanceBuffers()
{
	return mInstanceBuffers.get();
}

ID3D12CommandAllocator* FrameResource::GetDirectCommandAllocator()
{
	return mDirectCommandAllocator.Get();
}
ID3D12GraphicsCommandList* FrameResource::GetDirectCommandList()
{
	return mDirectCommandList.Get();
}
ID3D12CommandAllocator* FrameResource::GetBundleAllocator()
{
	return mBundleAllocator.Get();
}
ID3D12GraphicsCommandList* FrameResource::GetBundle()
{
	return mBundle.Get();
}

void FrameResource::SetFenceValue(UINT64 fenceValue)
{
	mFenceValue = fenceValue;
}
UINT64 FrameResource::GetFenceValue()
{
	return mFenceValue;
}

void FrameResource::ResetBundle()
{
	ThrowIfFailed(mBundle->Reset(mBundleAllocator.Get(), nullptr));
}