#include "stdafx.h"
#include "BillboardMat.h"
#include <Components\Transform.h>
#include <Components\Camera.h>
#include <Rendering\Mesh.h>
#include <SOIL/SOIL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

namespace snes
{
	BillboardMat::BillboardMat() : Material(BILLBOARD)
	{
		SetUniformSampler2D("albedo", 0);
		SetUniformSampler2D("normal", 1);
	}

	BillboardMat::BillboardMat(std::ifstream& params) : Material(BILLBOARD)
	{
		SetUniformSampler2D("albedo", 0);
		SetUniformSampler2D("normal", 1);
		std::string line;

		// First line of material file is the number of billboard directions
		if (std::getline(params, line))
		{
			m_directionCount = std::stoi(line);
		}

		float degreesPerBillboard = 360.0f / m_directionCount;

		for (int i = 0; i < m_directionCount; i++)
		{
			std::getline(params, line);
			std::string albedoPath = line + "a.png";
			
			GLuint albedoTexID = Material::GetTexture(albedoPath);
			if (albedoTexID >= 0)
			{
				m_albedoTexIDs.push_back(albedoTexID);
			}

			std::string normalPath = line + "n.png";
			GLuint normalTexID = Material::GetTexture(normalPath);
			if (normalTexID >= 0)
			{
				m_normalTexIDs.push_back(normalTexID);
			}
		}

		SetUniformBool("useNormalMap", m_normalTexIDs.size() == m_albedoTexIDs.size());
	}


	BillboardMat::~BillboardMat()
	{
	}

	void BillboardMat::PrepareForRendering(Transform& transform, Camera& camera, Mesh& mesh, RenderPass renderPass)
	{
		SetUniformVec3("centerWorldPos", transform.GetWorldPosition());
		SetUniformVec3("worldScale", transform.GetWorldScale());

		Material::PrepareForRendering(renderPass);

		uint texIDToShow = GetTextureToShow(transform, camera);

		glm::vec3 normal = GetBillboardForwardVector(texIDToShow, transform);
		normal;	// Rotate normal by transform normal
		SetUniformVec3("surfaceNormal", normal);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_albedoTexIDs[texIDToShow]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_normalTexIDs[texIDToShow]);
	}

	uint BillboardMat::GetTextureToShow(Transform& transform, Camera& camera)
	{
		// Find angle between object forward vector and camera-object vector
		glm::vec3 objForward = transform.GetForwardVector();
		glm::vec3 cameraToObject = camera.GetTransform().GetWorldPosition() - transform.GetWorldPosition();

		// Angle from -180 to 180
		float angle = SignedAngleBetween(objForward, cameraToObject, glm::vec3(0, 1, 0));
		// Angle from 0 - 360
		if (angle < 0)
		{
			angle = 180 + (180 + angle);
		}

		float degreesPerBillboard = 360.0f / m_directionCount;

		// Offset angle by half the delta (0 + d/2 to 30 + d/2)
		angle += degreesPerBillboard / 2.0f;

		int index = angle / degreesPerBillboard;

		return index;
	}

	float BillboardMat::SignedAngleBetween(glm::vec3 a, glm::vec3 b, glm::vec3 normal)
	{
		a = glm::normalize(a);
		b = glm::normalize(b);

		float angleBetween = glm::acos(glm::dot(a, b));

		float sign = glm::dot(normal, glm::cross(a, b)) >= 0 ? 1.0f : -1.0f;

		float signedAngle = angleBetween * sign;

		signedAngle = signedAngle * (180 / 3.14159);

		return signedAngle;
	}

	glm::vec3 BillboardMat::GetBillboardForwardVector(int index, Transform& transform)
	{
		// Return the 'forward' vector of the direction the billboard represents

		glm::vec3 forwardVector = transform.GetForwardVector();
		glm::vec3 rotationAxis = glm::vec3(0, 1, 0);
		float angle = (360 / m_directionCount) * index;
		glm::mat4 rotMat = glm::rotate(angle, rotationAxis);

		return rotMat * glm::vec4(forwardVector, 1.0f);
	}

	float BillboardMat::GetLODCost(Mesh& mesh)
	{
		int faceCount = mesh.GetNumFaces();
		int vertexCount = mesh.GetVertexCount();
		float faceCoeff = 1.0f;
		float vertexCoeff = 1.0f;
		return faceCount * faceCoeff + vertexCount * vertexCoeff;
	}

	float BillboardMat::GetBenefit()
	{
		// Create an estimate benefit heuristic from the billboard texture size
		int w, h;
		int miplevel = 0;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_albedoTexIDs[0]);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &w);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &h);
		return (w * h);
	}
}
