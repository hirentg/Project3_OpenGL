#pragma once
#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

#include <string>
#include <vector>

// Minimal data required for a mesh
struct Vertex
{
	glm::vec3 Position{};
	glm::vec3 Normal{};
	glm::vec2 TexCoords{};
	glm::vec3 Tangent{};
	glm::vec3 Bitangent{};
};

// Texture data
struct Texture
{
	unsigned int id{};
	std::string type{};		// e.g. diffuse or specular texture
	std::string path{};		// for optimization
};

class Mesh
{
private:
	// Render data
	unsigned int m_VAO{};
	unsigned int m_VBO{};
	unsigned int m_EBO{};

	// Helper function
	void setupMesh();

public:
	// Mesh data
	std::vector<Vertex> vertices{};
	std::vector<unsigned int> indices{};
	std::vector<Texture> textures{};

	// Constructor
	Mesh(const std::vector<Vertex>& vertice,
		const std::vector<unsigned int>& indice,
		const std::vector<Texture>& texture);

	// Draw
	void Draw(Shader& shader) const;

	unsigned int getVAO() const
	{
		return m_VAO;
	}
};

#endif // !MESH_H