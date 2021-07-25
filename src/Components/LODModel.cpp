#include "stdafx.h"
#include "LODModel.h"
#include "Camera.h"
#include <Core\FrameTime.h>
#include <Core\GameObject.h>
#include <Core\Input.h>
#include <Rendering\StandardModel.h>
#include <GL/glew.h>
#include <glm/gtx/euler_angles.hpp>
#include <algorithm>
#include <fstream>

namespace snes
{
	const float LODModel::STIPPLE_PATTERN[16] = {
		1.0f/17.0f,  9.0f/17.0f,  3.0f/17.0f,  11.0f/17.0f,
		13.0f/17.0f, 5.0f/17.0f,  15.0f/17.0f, 7.0f/17.0f,
		4.0f/17.0f,  12.0f/17.0f, 2.0f/17.0f,  10.0f/17.0f,
		16.0f/17.0f, 8.0f/17.0f,  14.0f/17.0f, 6.0f/17.0f
	};
	const float LODModel::TRANSITION_DURATION_S = 0.5f;

	std::vector<LODValue> LODModel::m_lodValues;
	std::vector<LODModel*> LODModel::m_lodModels;
	uint LODModel::m_instanceCount = 0;
	float LODModel::m_totalCost = 0;
	float LODModel::m_totalVerts = 0;
	float LODModel::m_totalFaces = 0;
	float LODModel::m_maxCost = 50000000;
	std::weak_ptr<GameObject> LODModel::m_referenceObj;
	bool LODModel::m_useReferenceObj = false;
	bool LODModel::m_useCostBenefit = false;
	
	void LODModel::Load(std::string modelName)
	{
		// Load the .lod file for this model
		std::string lodPath = modelName + ".lod";
		std::ifstream lodFile(lodPath, std::ios::in);

		if (!lodFile)
		{
			std::cout << "Error opening LOD file: " << lodPath << std::endl;
			return;
		}

		// Load each model file in the .lod file
		std::string line;
		while (std::getline(lodFile, line))
		{
			auto newModel = std::make_shared<StandardModel>();
			newModel->Load(line);
			m_models.push_back(newModel);
			m_costs.push_back(newModel->CalculateCost());
		}

		if (m_models.size() == 0)
		{
			// No meshes were loaded
			std::cout << "Error: No LOD models loaded for model name " << modelName << std::endl;
			return;
		}

		m_currentModel = m_models.size() - 1;
		m_lodModels.push_back(this);
	}

	int LODModel::GetCurrentLOD() const
	{
		if (m_meshes.size() == 0 || m_camera.expired())
		{
			return -1;
		}

		// Use heuristics to determine the best mesh to show
		glm::vec3 cameraPos;
		if (m_useReferenceObj)
		{
			cameraPos = m_referenceObj.lock()->GetTransform().GetWorldPosition();
		}
		else
		{
			cameraPos = m_camera.lock()->GetTransform().GetWorldPosition();
		}

		float distanceToCamera = glm::length(m_transform.GetWorldPosition() - cameraPos);
		float distancePerLOD = (m_distanceLow - m_distanceHigh) / m_meshes.size();

		uint lodToShow = (uint)std::abs((distanceToCamera + m_distanceHigh) / distancePerLOD);
		lodToShow = std::max(lodToShow, (uint)0);
		lodToShow = std::min(lodToShow, (m_meshes.size() - 1));

		// Return that mesh
		return lodToShow;
	}

	void LODModel::FixedLogic()
	{
		m_currentModel = 0;
		m_shownMeshCost = 0;
		if (m_models.size() > 1)
		{
			if (m_useCostBenefit)
			{
				CalculateEachLODValue();
			}
			else
			{
				m_currentModel = PickBestMesh();
			}
		}
	}

	int LODModel::PickBestMesh()
	{
		const float MIN_PIXELS_PER_POLYGON = 4;

		float normalizedMeshRadius = std::fmin(1.0f, GetScreenSizeOfMesh(m_models.size() - 1));
		float pixelMeshRadius = normalizedMeshRadius * 1024.0f / 2.0f;
		float circleArea = 3.14159f * (pixelMeshRadius * pixelMeshRadius);

		int bestIndex = m_models.size() - 1;
		int bestPixelsPerPolygon = 9999999;

		// Loop from lowest LOD to highest LOD and pick the best one to show below min_pixels_per_polygon
		for (int i = m_models.size() - 1; i >= 0; i--)
		{
			float numFaces = m_models[i]->GetMesh().lock()->GetNumFaces() / 2;
			float pixelsPerPolygon = (circleArea / numFaces);
			if (pixelsPerPolygon < bestPixelsPerPolygon && pixelsPerPolygon > MIN_PIXELS_PER_POLYGON)
			{
				bestIndex = i;
				bestPixelsPerPolygon = pixelsPerPolygon;
			}
		}

		return bestIndex;
	}

	void LODModel::MainLogic()
	{
		// Update LOD transition
		if (m_transitionRemainingS > 0.0f)
		{
			m_transitionRemainingS -= FrameTime::GetLastFrameDuration();
		}
	}

	void LODModel::MainDraw(RenderPass renderPass, Camera& camera)
	{
		if (renderPass == GEOMETRY_PASS)
		{
			if (m_currentModel != m_lastRenderedModel && m_transitionRemainingS <= 0.0f)
			{
				// The displayed mesh has changed - begin a transition
				m_transitioningFromModel = m_lastRenderedModel;	// Set the last mesh as one to fade out
				m_lastRenderedModel = m_currentModel;
				m_transitionRemainingS = TRANSITION_DURATION_S;
			}
		}

		if (m_transitionRemainingS > 0.0f)
		{
			glEnable(GL_POLYGON_STIPPLE);
			float opacity = 1.0f - (m_transitionRemainingS / TRANSITION_DURATION_S);
			GenerateStipplePattern(opacity, m_stipplePattern);
			glPolygonStipple(m_stipplePattern);
		}

		m_models[m_lastRenderedModel]->Render(renderPass, camera, m_transform);

		if (m_transitionRemainingS > 0.0f)
		{
			// Draw previously selected mesh if transitioning
			// Generate a stipple pattern that is the inverse of the one above
			InvertStipplePattern(m_stipplePattern);
			glPolygonStipple(m_stipplePattern);

			m_models[m_transitioningFromModel]->Render(renderPass, camera, m_transform);
		}

		glDisable(GL_POLYGON_STIPPLE);

		// Unbind the VBO and VAO
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	
	void LODModel::GenerateStipplePattern(float opacity, GLubyte patternOut[128])
	{
		// Generate a 32x32 stipple pattern from the 4x4 ordered dithering pattern and opacity
		for (int l = 0; l < 8; l++)
		{
			for (int k = 0; k < 4; k++)
			{
				for (int i = 0; i < 4; i++)
				{
					GLubyte eightBits = 0;
					for (int j = 0; j < 4; j++)
					{
						if (opacity > STIPPLE_PATTERN[(4 * k) + j])
						{
							eightBits = eightBits | (1 << j);
							eightBits = eightBits | (1 << (j + 4));
						}
					}
					patternOut[(16 * l) + (4 * k) + i] = eightBits;
				}
			}
		}
	}

	void LODModel::InvertStipplePattern(GLubyte patternOut[128])
	{
		// Invert the bits of the stipple pattern
		for (int i = 0; i < 128; i++)
		{
			patternOut[i] = patternOut[i] ^ 255;
		}
	}

	const std::weak_ptr<Mesh> LODModel::GetMesh(uint lodLevel) const
	{
		if (lodLevel < m_models.size())
		{
			return m_models[m_currentModel]->GetMesh();
		}
		else
		{
			// No valid mesh chosen, return null pointer
			return std::weak_ptr<Mesh>();
		}
	}

	int LODModel::CalculateEachLODValue()
	{
		if (m_models.size() > 1)
		{
			float size = GetScreenSizeOfMesh(m_models.size() - 1);
			float normalizedMeshRadius = std::fmin(1.0f, GetScreenSizeOfMesh(m_models.size() - 1));
			float pixelMeshRadius = normalizedMeshRadius * 1024.0f / 2.0f;
			float circleArea = 3.14159f * (pixelMeshRadius * pixelMeshRadius);

			if (size == 1.0f)
			{
				// show highest LOD
				float value = m_models[m_models.size() - 1]->CalculateLODValue(circleArea, *m_models[0]->GetMesh().lock());
				LODValue valueEntry = { this, 0, m_costs[0], value };
				m_lodValues.push_back(valueEntry);
			}

			int bestIndex = 0;
			float bestValue = 0;

			for (uint i = 0; i < m_models.size(); i++)
			{
				float value = m_models[i]->CalculateLODValue(circleArea, *m_models[0]->GetMesh().lock());

				LODValue valueEntry = { this, i, m_costs[i], value };
				m_lodValues.push_back(valueEntry);
			}
		}
		return 1;
	}

	float LODModel::GetScreenSizeOfMesh(int index)
	{
		// This code adapted from https://stackoverflow.com/questions/21648630/radius-of-projected-sphere-in-screen-space
		float fovy = m_camera.lock()->GetVerticalFoV();

		// Get the radius of the mesh's encapsulating sphere
		glm::vec3 worldScale = m_transform.GetWorldScale();
		float maxScale = std::max(std::max(worldScale.x, worldScale.y), worldScale.z);
		float r = m_models[index]->GetMesh().lock()->GetSize() * maxScale / 2.0f;

		// Get the distance between the camera and the mesh's origin
		/** Find distance from camera or distance from reference object? */
		glm::vec3 cameraPos;
		if (m_useReferenceObj)
		{
			cameraPos = m_referenceObj.lock()->GetTransform().GetWorldPosition();
		}
		else
		{
			cameraPos = m_camera.lock()->GetTransform().GetWorldPosition();
		}

		glm::vec3 meshCentre = m_models[index]->GetMesh().lock()->GetCentre();
		glm::vec3 meshPos = (glm::vec3)(m_transform.GetTRS() * glm::vec4(meshCentre, 1.0f));

		float d = glm::length(meshPos - cameraPos);
		if (r >= d)
		{
			return 1.0f;
		}

		// Find the "projected radius" of the mesh in normalized screen space
		float l = sqrtf(d*d - r*r);
		float pr = (1 / tanf(fovy / 2) * (r / l));
		return pr;
		// End of code adapted from https://stackoverflow.com/questions/21648630/radius-of-projected-sphere-in-screen-space
	}

	void LODModel::StartNewFrame()
	{
		m_lodValues.clear();
		m_lodValues.reserve(m_instanceCount * 3);
		m_totalCost = 0;
	}

	void LODModel::SortAndSetLODValues()
	{
		if (!m_useCostBenefit)
		{
			return;
		}

		bool printInfo = false;
		if (Input::GetKeyDown('-'))
		{
			m_maxCost -= 1000000;
			printInfo = true;
		}
		if (Input::GetKeyDown('='))
		{
			m_maxCost += 1000000;
			printInfo = true;
		}
		if (Input::GetKeyDown('o'))
		{
			m_useReferenceObj = !m_useReferenceObj;
		}

		// Sort m_lodValues by value
		std::sort(m_lodValues.begin(), m_lodValues.end(), [](const LODValue& a, const LODValue& b)
		{
			return a.value > b.value;
		});

		// Set all LODs to their smallest
		for (int meshIndex = 0; meshIndex < m_lodModels.size(); meshIndex++)
		{
			m_lodModels[meshIndex]->SetToLowestLOD();
		}

		uint i = 0;
		while (m_totalCost < m_maxCost && i < m_lodValues.size())
		{
			if (m_lodValues[i].model->SetCurrentLOD(m_lodValues[i]))
			{
				++i;
			}
			else
			{
				break;
			}
		}

		if (printInfo)
		{
			std::cout << std::fixed;
			std::cout << "Scene budget: " << m_maxCost << std::endl;
			std::cout << "Total cost: " << m_totalCost << std::endl;
			std::cout << "Total verts: " << m_totalVerts << std::endl;
			std::cout << "Total faces: " << m_totalFaces << std::endl;
		}
	}

	bool LODModel::SetCurrentLOD(LODValue& lodValue)
	{
		if (((m_totalCost - m_shownMeshCost) + lodValue.cost) > m_maxCost)
		{
			return false;
		}

		m_totalCost -= m_shownMeshCost;
		m_totalVerts -= m_shownMeshVerts;
		m_totalFaces -= m_shownMeshFaces;

		m_currentModel = lodValue.meshIndex;
		m_shownMeshCost = lodValue.cost;
		m_shownMeshVerts = m_models[m_currentModel]->GetMesh().lock()->GetVertexCount();
		m_shownMeshFaces = m_models[m_currentModel]->GetMesh().lock()->GetNumFaces();

		m_totalCost += m_shownMeshCost;
		m_totalVerts += m_shownMeshVerts;
		m_totalFaces += m_shownMeshFaces;
		
		return true;
	}

	void LODModel::SetCurrentLOD(uint index)
	{
		m_totalCost -= m_shownMeshCost;
		m_totalVerts -= m_shownMeshVerts;
		m_totalFaces -= m_shownMeshFaces;

		m_currentModel = index;
		m_shownMeshCost = m_costs[index];
		m_shownMeshVerts = m_models[m_currentModel]->GetMesh().lock()->GetVertexCount();
		m_shownMeshFaces = m_models[m_currentModel]->GetMesh().lock()->GetNumFaces();

		m_totalCost += m_shownMeshCost;
		m_totalVerts += m_shownMeshVerts;
		m_totalFaces += m_shownMeshFaces;
	}

	void LODModel::SetToLowestLOD()
	{
		m_totalCost -= m_shownMeshCost;
		m_totalVerts -= m_shownMeshVerts;
		m_totalFaces -= m_shownMeshFaces;

		m_currentModel = m_models.size() - 1;
		m_shownMeshCost = m_costs[m_models.size() - 1];
		m_shownMeshVerts = m_models[m_currentModel]->GetMesh().lock()->GetVertexCount();
		m_shownMeshFaces = m_models[m_currentModel]->GetMesh().lock()->GetNumFaces();

		m_totalCost += m_shownMeshCost;
		m_totalVerts += m_shownMeshVerts;
		m_totalFaces += m_shownMeshFaces;
	}
}
