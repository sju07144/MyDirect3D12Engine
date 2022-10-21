#pragma once
#include "Utility.h"

class Mesh
{
public:
	Mesh() = default;
	Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

	void ConfigureMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

	ID3D12Resource* GetVertexBuffer();
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView();

	ID3D12Resource* GetIndexBuffer();
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView();
	UINT GetIndexCount();
private:
	void CreateVertexBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void CreateIndexBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
private:
	std::vector<Vertex> mVertices;
	UINT vertexByteSize = 0;

	std::vector<uint32_t> mIndices;
	UINT indexByteSize = 0;
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT;

	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mVertexBufferUpload = nullptr;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mIndexBufferUpload = nullptr;
	D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};
};