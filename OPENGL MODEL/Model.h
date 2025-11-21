#pragma once
#ifndef MODEL_H
#define MODEL_H

#include "Mesh.h"
#include "Shader.h"
#include "stb_image.h"


#include <assimp/Importer.hpp>      // The main Importer class
#include <assimp/scene.h>           // The C-style data structures (scene, mesh, material)
#include <assimp/postprocess.h>   // Post-processing flags


class Model
{
private:
	// modal data
	std::vector<Mesh> m_meshes{};
	std::string m_directory{};

	std::vector<Texture> texture_loaded{};	// store loaded textures

	// load model with supported Assimp extensions from files and store the
	// resulting meshes in the mesh vector
	void loadModel(const std::string& path);

	// process a node in a recursive fashion. 
	// process each individual mesh located at the node and repeat this proces on its children note (if any)
	void processNode(aiNode* node, const aiScene* scene);

	// process Assimp data to our Mesh class
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);

	// check all material texture of a given type and load the texture
	// if they are not loaded yet	
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
		std::string typeName);

public:
	Model(const std::string& path)
	{
		loadModel(path);
	}

	// Draw the model (all of its meshes)
	void Draw(Shader& shader)
	{
		for (unsigned int i{ 0 }; i < m_meshes.size(); ++i)
		{
			m_meshes[i].Draw(shader);
		}
	}
};


void Model::loadModel(const std::string& path)
{
	Assimp::Importer import{};

	// flipUVs flip the y axis 
	// (normally the (0,0) coordinate of texture is at the top left)
	const aiScene* scene{ import.ReadFile(path, aiProcess_Triangulate
										| aiProcess_FlipUVs) };

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << '\n';
		return;
	}

	// retrieve the directory path of a filepath
	m_directory = path.substr(0, path.find_last_of('/'));

	// process Assimp root node recursively
	processNode(scene->mRootNode, scene);
}

unsigned int TextureFromFile(const char* path, const std::string& directory);


void Model::processNode(aiNode* node, const aiScene* scene)
{
	// process all the nodes meshes (if any)
	for (unsigned int i{ 0 }; i < node->mNumMeshes; ++i)
	{
		// node object only contains the indices to index the 
		// actual object in the scene. 
		// The scene contains all the data, node is just to keep things organized
		aiMesh* mesh{ scene->mMeshes[node->mMeshes[i]] };
		m_meshes.push_back(processMesh(mesh, scene));
	}

	// Do the same for each children node
	for (unsigned int i{ 0 }; i < node->mNumChildren; ++i)
	{
		processNode(node->mChildren[i], scene);
	}
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	// general idea: Access each of the mesh's relevant properties and store 
	// them in our object

	std::vector<Vertex> vertices{};
	std::vector<unsigned int> indices{};
	std::vector<Texture> textures{};

	// iterate through each of the mesh vertices
	for (unsigned int i{ 0 }; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex{};
		// process vertex positions, normals, and texture coordinates

		// position
		glm::vec3 vector{};
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = vector;

		// normals
		if (mesh->HasNormals())
		{
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = vector;
		}


		// check if texture coordiantes exist
		if (mesh->mTextureCoords[0])
		{
			glm::vec2 vec{};
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;

			vertex.TexCoords = vec;
		}

		else
			vertex.TexCoords = glm::vec2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}

	// process indices
	// loop through all the faces in the mesh and get the faces indices
	for (unsigned int i{ 0 }; i < mesh->mNumFaces; ++i)
	{
		aiFace face{ mesh->mFaces[i] };
		for (unsigned int j{ 0 }; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}


	// process materials
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		std::vector<Texture> diffuseMaps{ loadMaterialTextures(material,
									aiTextureType_DIFFUSE, "texture_diffuse") };

		std::vector<Texture> specularMaps{ loadMaterialTextures(material,
									aiTextureType_SPECULAR, "texture_specular") };

		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

	}

	return Mesh(vertices, indices, textures);
}


std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type,
	std::string typeName)
{
	std::vector<Texture> textures{};

	for (unsigned int i{ 0 }; i < mat->GetTextureCount(type); ++i)
	{
		aiString str{};
		mat->GetTexture(type, i, &str);
		bool skip{ false };

		// check if a texture has been loaded
		for (unsigned int j{ 0 }; j < texture_loaded.size(); ++j)
		{
			if (std::strcmp(texture_loaded[j].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(texture_loaded[j]);
				skip = true;
				break;
			}
		}

		if (!skip)
		{
			Texture texture{};
			texture.id = TextureFromFile(str.C_Str(), m_directory);
			texture.type = typeName;
			texture.path = str.C_Str();

			textures.push_back(texture);
			texture_loaded.push_back(texture);	// add to loaded textures
		}
	}

	return textures;
}


// read texture from file
unsigned int TextureFromFile(const char* path, const std::string& directory)
{
	std::string filename = std::string{ path };
	filename = directory + '/' + filename;

	unsigned int textureID{};
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data{ stbi_load(filename.c_str(), &width, &height, &nrComponents, 0)};
	if (data)
	{
		GLenum format{};
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);

		stbi_image_free(data);
	}

	else
	{
		std::cout << "Failed to load at path: " << path << '\n';
		stbi_image_free(data);
	}

	return textureID;
}

#endif // !1
