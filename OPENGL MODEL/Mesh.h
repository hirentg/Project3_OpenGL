#pragma once
#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include <glad/glad.h>


#include <string>
#include <vector>
#include <cstddef>  // for offsetof


// Minimal data required for a mesh
struct Vertex
{
	glm::vec3 Position{};
	glm::vec3 Normal{};
	glm::vec2 TexCoords{};

};

// Texture data
struct Texture
{
	unsigned int id{};		
	std::string type{};		// e.g. diffuse or specular texture
	std::string path{};		// for optimization - store the texture path to
							// compare with other texture
};


class Mesh
{
private:
	// render data
	unsigned int m_VAO{};
	unsigned int m_VBO{};
	unsigned int m_EBO{};
	
	void setupMesh();

public:
	// mesh data
	std::vector<Vertex> vertices{};
	std::vector<unsigned int> indices{};
	std::vector<Texture> textures{};

	// constructor
	Mesh(const std::vector<Vertex>& vertice, std::vector<unsigned int> indice, std::vector<Texture> texture)
	{
		vertices = vertice;
		indices = indice;
		textures = texture;

		setupMesh();
	}


	void Draw(Shader& shader) const;

};


void Mesh::setupMesh()
{
	glGenBuffers(1, &m_VBO);
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_EBO);

	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(),
		GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(),
		GL_STATIC_DRAW);

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);

	// vertex normals
	// macro offsetof(s, m) takes its 1st argument as a struct, 2nd argument a variable of 
	// that struct. It returns the offset of that variable from the start of the struct
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	glEnableVertexAttribArray(1);

	// vertex texture coordinates
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	glEnableVertexAttribArray(2);


}


void Mesh::Draw(Shader& shader) const
{
	// define N number of texture and specular textures
	unsigned int diffuseN{ 1 };
	unsigned int specularN{ 1 };

	for (unsigned int i{ 0 }; i < textures.size(); ++i)
	{
		// Activate the corresponding texture unit before binding
		glActiveTexture(GL_TEXTURE0 + i);
		// retrieve texture number
		std::string number{};
		std::string name{ textures[i].type };	

		if (name == "texture_diffuse")
			number = std::to_string(diffuseN++);	// assign first, then increment

		else if (name == "texture_specular")
			number = std::to_string(specularN++);

		shader.setInt(("material." + name + number).c_str(), i);
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}

	// draw mesh
	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// set everything back to default once configured
	glActiveTexture(GL_TEXTURE0);
}
#endif // !MESH_H
