#pragma once
#include "Stdafx.h"
#include "Utility.h"

class Mesh;

class BasicGeometryGenerator
{
public:
	BasicGeometryGenerator() = default;

	Mesh CreateBox(float width, float height, float depth);
private:
	std::vector<Vertex> mVertices;
	UINT mVertexCount;

	std::vector<uint32_t> mIndices;
	UINT mIndexCount;
};