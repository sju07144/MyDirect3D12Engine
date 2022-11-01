#pragma once
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "Stdafx.h"

class Mesh;
class Texture;

class Model
{
public:
	Model() = default;
	Model(const std::string& path);

	void LoadModel(const std::string& path);

	std::vector<Mesh> GetMeshes();
	std::vector<Texture> GetRawTextures();
private:
	void ProcessNode(aiNode* node, const aiScene* scene);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

	void LoadTexture(aiMaterial* mat, aiTextureType textureType);
private:
	std::vector<Mesh> mMeshes;
	std::vector<Texture> mRawTextures;
};