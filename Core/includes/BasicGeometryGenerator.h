#pragma once
#include "Stdafx.h"
#include "Utility.h"

class Mesh;

class BasicGeometryGenerator
{
public:
	BasicGeometryGenerator() = default;

	Mesh CreateBox(float width, float height, float depth);
	Mesh CreateGrid(float width, float depth, uint32_t m, uint32_t n);
	Mesh CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount);
private:
	std::vector<Vertex> mVertices;
	UINT mVertexCount;

	std::vector<uint32_t> mIndices;
	UINT mIndexCount;
};