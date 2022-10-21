#include "Command.h"

Command::Command()
{ }
Command::Command(D3D12_COMMAND_LIST_TYPE commandListType)
	: mCommandListType(commandListType)
{ }

void Command::SetCommandListType(D3D12_COMMAND_LIST_TYPE commandListType)
{
	assert(mCommandListType != commandListType);
	mCommandListType = commandListType;
}

void Command::CreateCommandAllocator(ID3D12Device* device)
{
	ThrowIfFailed(device->CreateCommandAllocator(mCommandListType, IID_PPV_ARGS(&mCommandAllocator)));
}
void Command::CreateCommandQueue(ID3D12Device* device)
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = mCommandListType;

	ThrowIfFailed(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&mCommandQueue)));
}
void Command::CreateGraphicsCommandList(ID3D12Device* device)
{
	ThrowIfFailed(device->CreateCommandList(0, mCommandListType,
		mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));

	mCommandList->Close(); // close before use.

	mCommandList->Reset(mCommandAllocator.Get(), nullptr);
}

ID3D12CommandAllocator* Command::GetCommandAllocator()
{
	return mCommandAllocator.Get();
}
ID3D12CommandQueue* Command::GetCommandQueue()
{
	return mCommandQueue.Get();
}
ID3D12GraphicsCommandList* Command::GetCommandList()
{
	return mCommandList.Get();
}