#pragma once
#include "Utility.h"

class Command
{
public:
	Command();
	Command(D3D12_COMMAND_LIST_TYPE commandListType);

	void SetCommandListType(D3D12_COMMAND_LIST_TYPE commandListType);

	void CreateCommandAllocator(ID3D12Device* device);
	void CreateCommandQueue(ID3D12Device* device);
	void CreateGraphicsCommandList(ID3D12Device* device);

	ID3D12CommandAllocator* GetCommandAllocator();
	ID3D12CommandQueue* GetCommandQueue();
	ID3D12GraphicsCommandList* GetCommandList();
private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator = nullptr;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue = nullptr;
	
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList = nullptr;
	D3D12_COMMAND_LIST_TYPE mCommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT; // default type is direct command list type
	// std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mGraphicsCommandLists;
	// UINT mGraphicsCommandListCount;
};