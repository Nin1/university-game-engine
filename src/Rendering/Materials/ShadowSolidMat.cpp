#include "stdafx.h"
#include "ShadowSolidMat.h"
#include <Rendering\Mesh.h>

namespace snes
{
	ShadowSolidMat::ShadowSolidMat() : Material(SHADOW_SOLID)
	{
	}

	ShadowSolidMat::ShadowSolidMat(std::ifstream& params) : Material(SHADOW_SOLID)
	{
	}

	ShadowSolidMat::~ShadowSolidMat()
	{
	}

	void ShadowSolidMat::ApplyTransformUniforms(glm::mat4& model, glm::mat4& view, glm::mat4& proj, glm::mat4& projView)
	{
		SetUniformMat4("depthMVP", proj * view * model);
		Material::ApplyTransformUniforms(model, view, proj, projView);
	}

	float ShadowSolidMat::GetLODCost(Mesh& mesh)
	{
		int faceCount = mesh.GetNumFaces();
		int vertexCount = mesh.GetVertexCount();
		float faceCoeff = 1.0f;
		float vertexCoeff = 1.0f;
		return faceCount * faceCoeff + vertexCount * vertexCoeff;
	}
}
