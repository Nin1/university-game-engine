#include "stdafx.h"
#include "SilhouetteTessellatedMat.h"
#include "Components\Camera.h"
#include <Core\Input.h>
#include <Rendering\Mesh.h>
#include <SOIL/SOIL.h>
#include <algorithm>

namespace snes
{
	bool SilhouetteTessellatedMat::m_useTessellation = true;

	SilhouetteTessellatedMat::SilhouetteTessellatedMat() : Material(SILHOUETTE_TESSELLATED_TEXTURED)
	{
		SetUniformSampler2D("tex1", 0);
		SetUniformSampler2D("dispMap", 1);
		SetUniformFloat("innerTessLevel", m_maxInnerTessLevel);
		SetUniformFloat("outerTessLevel", m_maxOuterTessLevel);
		SetUniformFloat("magnitude", m_displacementMagnitude);
		m_usePatches = true;
		glGenQueries(1, &m_queryID);
	}

	SilhouetteTessellatedMat::SilhouetteTessellatedMat(std::ifstream& params) : Material(SILHOUETTE_TESSELLATED_TEXTURED)
	{
		SetUniformSampler2D("tex1", 0);
		SetUniformSampler2D("dispMap", 1);
		SetUniformFloat("innerTessLevel", m_maxInnerTessLevel);
		SetUniformFloat("outerTessLevel", m_maxOuterTessLevel);
		SetUniformFloat("magnitude", m_displacementMagnitude);
		SetUniformBool("hasDispMap", false);
		m_usePatches = true;
		glGenQueries(1, &m_queryID);

		params >> m_maxInnerTessLevel;
		m_maxOuterTessLevel = m_maxInnerTessLevel;
		SetUniformFloat("innerTessLevel", m_maxInnerTessLevel);
		SetUniformFloat("outerTessLevel", m_maxOuterTessLevel);

		if (params.eof())
		{
			return;
		}

		params >> m_pixelsPerPolygon;

		if (params.eof())
		{
			return;
		}

		std::string texturePath;
		params >> texturePath;

		m_textureID = Material::GetTexture(texturePath);

		if (params.eof())
		{
			return;
		}

		params >> texturePath;

		m_dispMapID = Material::GetTexture(texturePath);

		SetUniformBool("hasDispMap", true);

		if (params.eof())
		{
			return;
		}

		params >> m_displacementMagnitude;
		SetUniformFloat("magnitude", m_displacementMagnitude);
	}


	SilhouetteTessellatedMat::~SilhouetteTessellatedMat()
	{
	}

	void SilhouetteTessellatedMat::PrepareForRendering(Transform& transform, Camera& camera, Mesh& mesh, RenderPass renderPass)
	{
		if (m_useTessellation && renderPass == GEOMETRY_PASS)
		{
			uint64 pixelsRenderedLastFrame = 16;
			//glGetQueryObjectui64v(m_queryID, GL_QUERY_RESULT, &pixelsRenderedLastFrame);
			
			float normalizedMeshRadius = std::fmin(1.0f, GetScreenSizeOfMesh(transform, camera, mesh));
			float pixelMeshRadius = normalizedMeshRadius * 1024.0f / 2.0f;
			float circleArea = 3.14159f * (pixelMeshRadius * pixelMeshRadius);
			float pixelsPerPolygon = (int)(circleArea / mesh.GetNumFaces());
			

			//float pixelsPerPolygon = std::sqrtf(pixelsRenderedLastFrame / mesh.GetNumFaces());

			// Adjust displacement amount based on base polygon size size
			float pixelThresholdDivider = 2.0f;

			float tessAmount;
			if (pixelsPerPolygon < (m_pixelsPerPolygon / pixelThresholdDivider))
			{
				SetUniformFloat("innerTessLevel", 1);
				SetUniformFloat("outerTessLevel", 1);
				SetUniformFloat("magnitude", 0);
			}
			else if (pixelsPerPolygon >(m_pixelsPerPolygon / pixelThresholdDivider) && pixelsPerPolygon < m_pixelsPerPolygon && m_dispMapID != -1)
			{
				// Fade in the displacement map
				SetUniformFloat("innerTessLevel", m_maxInnerTessLevel);
				SetUniformFloat("outerTessLevel", m_maxOuterTessLevel);
				// Displacement = percentage from ppp/2 to ppp
				float halfPixels = m_pixelsPerPolygon / pixelThresholdDivider;
				float pixelsToFull = pixelsPerPolygon - halfPixels;
				float percentage = pixelsToFull / halfPixels;
				SetUniformFloat("magnitude", m_displacementMagnitude * percentage);
			}
			else if (pixelsPerPolygon > m_pixelsPerPolygon)
			{
				if (m_dispMapID == -1)
				{
					tessAmount = pixelsPerPolygon / m_pixelsPerPolygon;
					SetUniformFloat("innerTessLevel", std::min(m_maxInnerTessLevel, tessAmount));
					SetUniformFloat("outerTessLevel", std::min(m_maxOuterTessLevel, tessAmount));
				}
				else
				{
					SetUniformFloat("innerTessLevel", m_maxInnerTessLevel);
					SetUniformFloat("outerTessLevel", m_maxOuterTessLevel);
					SetUniformFloat("magnitude", m_displacementMagnitude);
				}
			}


			if (Input::GetKeyDown('t'))
			{
				std::cout << "Pixels per polygon: " << pixelsPerPolygon << std::endl;
				std::cout << "Desired pixels per polygon: " << m_pixelsPerPolygon << std::endl;
				std::cout << "Query ID: " << m_queryID << std::endl << std::endl;
			}
		}
		else
		{
			SetUniformFloat("innerTessLevel", 1);
			SetUniformFloat("outerTessLevel", 1);
		}

		SetUniformVec3("viewPos", camera.GetTransform().GetWorldPosition());

		Material::PrepareForRendering(renderPass);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureID);
		glActiveTexture(GL_TEXTURE1);
		if (m_dispMapID != -1)
		{
			glBindTexture(GL_TEXTURE_2D, m_dispMapID);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_textureID);
		}
		glPatchParameteri(GL_PATCH_VERTICES, 6);	// @TODO: This is 6 here for neighbouring vertices, but should be 3 for non-silhouette based shaders. Make new materials.

		if (renderPass == GEOMETRY_PASS)
		{
			//glBeginQuery(GL_SAMPLES_PASSED, m_queryID);
		}
	}

	void SilhouetteTessellatedMat::PostRendering()
	{
		//glEndQuery(GL_SAMPLES_PASSED);
	}

	void SilhouetteTessellatedMat::SetTexture(const char* texturePath)
	{
		m_textureID = SOIL_load_OGL_texture(
			texturePath,
			SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID,
			SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
		);
	}

	float SilhouetteTessellatedMat::GetScreenSizeOfMesh(Transform& transform, Camera& camera, Mesh& mesh)
	{
		// This code adapted from https://stackoverflow.com/questions/21648630/radius-of-projected-sphere-in-screen-space
		float fovy = camera.GetVerticalFoV();
		fovy *= 3.14159f / 180;

		// Get the radius of the mesh's encapsulating sphere
		glm::vec3 worldScale = transform.GetWorldScale();
		float maxScale = std::max(std::max(worldScale.x, worldScale.y), worldScale.z);
		float r = mesh.GetSize() * maxScale / 2.0f;

		// Get the distance between the camera and the mesh's origin
		/** Find distance from camera or distance from reference object? */
		glm::vec3 cameraPos;
		//		if (m_useReferenceObj)
		//		{
		//			cameraPos = m_referenceObj.lock()->GetTransform().GetWorldPosition();
		//		}
		//		else
		{
			cameraPos = camera.GetTransform().GetWorldPosition();
		}

		glm::vec3 meshPos = (glm::vec3)(transform.GetTRS() * glm::vec4(mesh.GetCentre(), 1.0f));

		float d = glm::length(meshPos - cameraPos);
		if (r >= d)
		{
			return 1.0f;
		}

		// Find the "projected radius" of the mesh in normalized screen space
		float l = sqrtf(d*d - r*r);
		float pr = 1 / tanf(fovy) * r / l;
		return pr;
		// End of code adapted from https://stackoverflow.com/questions/21648630/radius-of-projected-sphere-in-screen-space
	}

	float SilhouetteTessellatedMat::GetLODCost(Mesh& mesh)
	{
		int faceCount = mesh.GetNumFaces();
		int vertexCount = mesh.GetVertexCount();
		float faceCoeff = 1.0f;
		float vertexCoeff = 1.0f;
		return faceCount * faceCoeff + vertexCount * vertexCoeff;
	}
}
