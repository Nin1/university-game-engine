#pragma once
#include "..\Material.h"
#include <fstream>

namespace snes
{
	/** Billboard Material
	  * A material always faces the camera */
	class BillboardMat : public Material
	{
	public:
		BillboardMat();
		BillboardMat(std::ifstream& params);
		~BillboardMat();

		void PrepareForRendering(Transform& transform, Camera& camera, Mesh& mesh, RenderPass renderPass) override;

		/** Determine which billboard to show based on the camera position and direction
		  * @return the index of the billboard to show */
		uint GetTextureToShow(Transform& transform, Camera& camera);

		/** @return the approximate cost to render the given mesh */
		float GetLODCost(Mesh& mesh) override;
		float GetBenefit() override;

	private:
		float SignedAngleBetween(glm::vec3 a, glm::vec3 b, glm::vec3 normal);
		glm::vec3 GetBillboardForwardVector(int index, Transform& transform);

		GLuint m_textureID = 0;
		GLuint m_normalTextureID = 0;
		/** The size of the billboard in world units (before scale) */
		int m_directionCount;

		std::vector<GLuint> m_albedoTexIDs;
		std::vector<GLuint> m_normalTexIDs;
		std::vector<glm::vec3> m_surfaceNormals;
	};
}
