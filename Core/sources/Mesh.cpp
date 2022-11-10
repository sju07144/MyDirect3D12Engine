#include "../includes/Mesh.h"
using namespace DirectX;

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
	D3D_PRIMITIVE_TOPOLOGY primitiveType)
	: mVertices(vertices), mIndices(indices), mPrimitiveType(primitiveType)
{
	vertexByteSize = (UINT)mVertices.size() * sizeof(Vertex);
	indexByteSize = (UINT)mIndices.size() * sizeof(uint32_t);
}

void Mesh::ConfigureMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	CreateVertexBuffer(device, commandList);
	CreateIndexBuffer(device, commandList);

	CreateBoundingBox();
	CreateBoundingSphere();
}

ID3D12Resource* Mesh::GetVertexBuffer()
{
	return mVertexBuffer.Get();
}
D3D12_VERTEX_BUFFER_VIEW Mesh::GetVertexBufferView()
{
	return mVertexBufferView;
}

ID3D12Resource* Mesh::GetIndexBuffer()
{
	return mIndexBuffer.Get();
}
D3D12_INDEX_BUFFER_VIEW Mesh::GetIndexBufferView()
{
	return mIndexBufferView;
}
UINT Mesh::GetIndexCount()
{
	return static_cast<UINT>(mIndices.size());
}

D3D_PRIMITIVE_TOPOLOGY Mesh::GetPrimitiveType()
{
	return mPrimitiveType;
}

DirectX::BoundingBox Mesh::GetBoundingBox()
{
	return mBoundingBox;
}
DirectX::BoundingSphere Mesh::GetBoundingSphere()
{
	return mBoundingSphere;
}

void Mesh::CreateVertexBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mVertexBuffer = D3D12Utility::CreateDefaultBuffer(device, commandList,
		mVertices.data(), vertexByteSize, mVertexBufferUpload);

	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.SizeInBytes = vertexByteSize;
	mVertexBufferView.StrideInBytes = sizeof(Vertex);
}
void Mesh::CreateIndexBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	mIndexBuffer = D3D12Utility::CreateDefaultBuffer(device, commandList,
		mIndices.data(), indexByteSize, mIndexBufferUpload);

	mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
	mIndexBufferView.Format = indexFormat;
	mIndexBufferView.SizeInBytes = indexByteSize;
}

void Mesh::CreateBoundingBox()
{
	XMVECTOR vMin;
	XMVECTOR vMax;
	XMVECTOR position;

	for (const auto& vertex : mVertices)
	{
		vMin = XMLoadFloat3(&mVertexMin);
		vMax = XMLoadFloat3(&mVertexMax);
		position = XMLoadFloat3(&vertex.position);

		vMin = XMVectorMin(vMin, position);
		vMax = XMVectorMax(vMax, position);

		XMStoreFloat3(&mVertexMin, vMin);
		XMStoreFloat3(&mVertexMax, vMax);
	}

	vMin = XMLoadFloat3(&mVertexMin);
	vMax = XMLoadFloat3(&mVertexMax);

	mBoundingBox.CreateFromPoints(mBoundingBox, vMin, vMax);
}
void Mesh::CreateBoundingSphere()
{
	mBoundingSphere.CreateFromBoundingBox(mBoundingSphere, mBoundingBox);
}