#include "stdafx.h"
#include "StandardModel.h"
#include <Components\Camera.h>
#include <Core\FrameTime.h>
#include <Core\GameObject.h>
#include <Core\Input.h>
#include <Rendering\Materials\BillboardMat.h>
#include <GL/glew.h>
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>
#include <fstream>

namespace snes
{	
	void StandardModel::Load(std::string fileName)
	{
		// Load the file for this model
		std::ifstream modelFile(fileName, std::ios::in);

		if (!modelFile)
		{
			std::cout << "Error opening StandardModel file: " << fileName << std::endl;
			return;
		}

		// Read mesh file
		std::string line;
		std::getline(modelFile, line);
		m_mesh = Mesh::GetMesh(line.c_str());
		if (!m_mesh)
		{
			std::cout << "Error opening mesh file: " << line << std::endl;
			return;
		}

		// Create material from file
		m_material = Material::CreateMaterial(modelFile);
		if (!m_material)
		{
			std::cout << "Error opening material file: " << line << std::endl;
			return;
		}

		if (m_material->RequiresNeighbourData())
		{
			std::cout << "Generating neighbour data for model: " << fileName << std::endl << "Please wait, this may take a while..." << std::endl;
			m_mesh->GenNeighbourData();
		}

		if (m_queryID == -1)
		{
			glGenQueries(1, &m_queryID);
		}
	}

	void StandardModel::Render(RenderPass renderPass, Camera& camera, Transform& transform)
	{
		Material* material = m_material.get();

		if (renderPass != SHADOW_PASS)
		{
			//glBeginQuery(GL_SAMPLES_PASSED, m_queryID);
		}

		this->PrepareTransformUniforms(camera, material, transform);
		material->PrepareForRendering(transform, camera, *m_mesh, renderPass);
		m_mesh->PrepareForRendering();

		// Draw the mesh
		if (material->GetUsePatches())
		{
			glDrawArrays(GL_PATCHES, 0, m_mesh->GetVertexCount());
		}
		else
		{
			glDrawArrays(GL_TRIANGLES, 0, m_mesh->GetVertexCount());
		}
		//glEndQuery(GL_SAMPLES_PASSED);

		material->PostRendering();

		// Unbind the VBO and VAO
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void StandardModel::PrepareTransformUniforms(Camera& camera, Material* mat, Transform& transform)
	{
		glm::mat4 modelMat = transform.GetTRS();
		glm::mat4 viewMat = camera.GetViewMatrix();
		glm::mat4 projMat = camera.GetProjMatrix();
		glm::mat4 projViewMat = camera.GetProjViewMatrix();

		mat->ApplyTransformUniforms(modelMat, viewMat, projMat, projViewMat);
	}

	float StandardModel::CalculateLODValue(float meshSize, const Mesh& baseMesh)
	{
		//glGetQueryObjectiv(m_queryID, GL_QUERY_RESULT, &m_pixelsRenderedLastFrame);

		uint numVerts = m_mesh->GetVertexCount() * m_material->GetVertexMultiplier();	// @TODO: Calculate a proper cost heuristic

		float baseError = 0.5f;
		float accuracy = 1.0f - ((baseError / numVerts)); // *(baseError / numFaces)); //1 - (BaseError / number of faces)^2
		float importance = 1.0f;
		float focus = 1.0f; // Proportional to distance from object to screen center
		float motion = 1.0f; // proportional to the ratio of the object's apparent speed to the size of an average polygon
		float material = m_material->GetBenefit(); // Bonus heuristic to allow a material to determine additional benefit i.e. billboards

		float benefit = meshSize * accuracy * importance * focus * motion * material; // * hysteresis

		float value = benefit / CalculateCost();
		return value;
	}

	float StandardModel::CalculateCost()
	{
		// @TODO: Replace '1' with cost coefficient from material
		return m_material->GetLODCost(*m_mesh);
	}
}
