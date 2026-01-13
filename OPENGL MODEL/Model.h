#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include <memory>

#include <assimp/Importer.hpp>      
#include <assimp/scene.h>           
#include <assimp/postprocess.h>   

#include "Mesh.h"
#include "Shader.h"

class Model
{
public:
	// Model data
	std::vector<Mesh> m_meshes;
	std::string m_directory;
	std::vector<Texture> texture_loaded;

	// Constructor
	Model(const std::string& path);

	// Draw
	void Draw(Shader& shader);

private:
	// Helper functions
	void loadModel(const std::string& path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};


unsigned int TextureFromFile(const char* path, const std::string& directory);

void modelLoading(std::unique_ptr<Model>& model, std::string& path, float& scale);

#endif