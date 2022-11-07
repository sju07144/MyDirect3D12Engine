#include "../includes/Mesh.h"
#include "../includes/Model.h"
#include "../includes/Texture.h"

Model::Model(const std::string& path)
{
	LoadModel(path);
}

void Model::LoadModel(const std::string& path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		throw std::runtime_error("Cannot read the model!");

	ProcessNode(scene->mRootNode, scene);
}

std::vector<Mesh> Model::GetMeshes()
{
	return mMeshes;
}
std::vector<Texture> Model::GetRawTextures()
{
	return mRawTextures;
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
	for (UINT i = 0; i < node->mNumMeshes; i++)
	{
		auto meshIndex = node->mMeshes[i];
		mMeshes.push_back(ProcessMesh(scene->mMeshes[meshIndex], scene));
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
		ProcessNode(node->mChildren[i], scene);
}
Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	vertices.reserve(mesh->mNumVertices);
	indices.reserve(mesh->mNumVertices);
	
	for (UINT i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		vertex.position.x = mesh->mVertices[i].x;
		vertex.position.y = mesh->mVertices[i].y;
		vertex.position.z = mesh->mVertices[i].z;

		vertex.normal.x = mesh->mNormals[i].x;
		vertex.normal.y = mesh->mNormals[i].y;
		vertex.normal.z = mesh->mNormals[i].z;

		if (mesh->mTextureCoords[0])
		{
			vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
			vertex.texCoord.y = mesh->mTextureCoords[0][i].y;
		}
		else
			vertex.texCoord = DirectX::XMFLOAT2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		LoadTexture(material, aiTextureType_DIFFUSE);
	}

	return Mesh(vertices, indices);
}

void Model::LoadTexture(aiMaterial* mat, aiTextureType textureType)
{
	
	UINT textureCount = mat->GetTextureCount(textureType);

	for (UINT i = 0; i < textureCount; i++)
	{
		Texture texture;
		aiString path;

		mat->GetTexture(textureType, i, &path); // revise path

		std::string pathNameUTF8;
		pathNameUTF8.assign(path.C_Str());

		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
		std::wstring pathName = converter.from_bytes(pathNameUTF8);

		texture.SetTextureFilename(pathName);

		mRawTextures.push_back(texture);
	}
}
