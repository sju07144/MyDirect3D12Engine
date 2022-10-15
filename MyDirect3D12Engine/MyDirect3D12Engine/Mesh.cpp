#include "Mesh.h"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices)
	: mVertices(vertices), mIndices(indices)
{
	vertexByteSize = mVertices.size() * sizeof(Vertex);
	indexByteSize = mIndices.size() * sizeof(uint16_t);
}

ID3D12Resource* Mesh::GetVertexBuffer()
{
	return mVertexBuffer.Get();
}
D3D12_VERTEX_BUFFER_VIEW Mesh::GetVertexBufferView()
{
	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = vertexByteSize;
	mVertexBufferView.StrideInBytes = sizeof(Vertex);

	return mVertexBufferView;
}

ID3D12Resource* Mesh::GetIndexBuffer()
{
	return mIndexBuffer.Get();
}
D3D12_INDEX_BUFFER_VIEW Mesh::GetIndexBufferView()
{
	mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	mIndexBufferView.Format = indexFormat;
	mIndexBufferView.SizeInBytes = indexByteSize;

	return mIndexBufferView;
}

void Mesh::CreateVertexBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mVertexBuffer = D3D12Utility::CreateDefaultBuffer(device, commandList,
		mVertices.data(), vertexByteSize, mVertexBufferUpload);
}
void Mesh::CreateIndexBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mIndexBuffer = D3D12Utility::CreateDefaultBuffer(device, commandList,
		mIndices.data(), indexByteSize, mIndexBufferUpload);
}
