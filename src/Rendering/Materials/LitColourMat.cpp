#include "stdafx.h"
#include "LitColourMat.h"
#include <Rendering\Mesh.h>

namespace snes
{
	LitColourMat::LitColourMat() : Material(LIT_COLOUR)
	{
		// Default to magenta
		glm::vec3 colour;
		colour.r = 1.0;
		colour.g = 0.0;
		colour.b = 1.0;
		SetUniformVec3("colour", colour);
	}

	LitColourMat::LitColourMat(std::ifstream& params) : Material(LIT_COLOUR)
	{
		std::string line;
		glm::vec3 colour;

		std::getline(params, line);
		colour.r = std::stoi(line) / 255.0f;

		std::getline(params, line);
		colour.g = std::stoi(line) / 255.0f;

		std::getline(params, line);
		colour.b = std::stoi(line) / 255.0f;

		SetUniformVec3("colour", colour);
	}


	LitColourMat::~LitColourMat()
	{
	}

	void LitColourMat::SetColour(glm::vec3 colour)
	{
		SetUniformVec3("colour", colour);
	}

	float LitColourMat::GetLODCost(Mesh& mesh)
	{
		int faceCount = mesh.GetNumFaces();
		int vertexCount = mesh.GetVertexCount();
		float faceCoeff = 0.5f;
		float vertexCoeff = 1.0f;
		return faceCount * faceCoeff + vertexCount * vertexCoeff;
	}
}
