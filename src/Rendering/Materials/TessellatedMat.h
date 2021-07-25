#pragma once
#include "..\Material.h"
#include <fstream>

namespace snes
{
	/** Tessellated Material
	  * Renders a textured mesh with tessellation. */
	class TessellatedMat : public Material
	{
	public:
		TessellatedMat();
		TessellatedMat(std::ifstream& params);
		~TessellatedMat();
		
		void PrepareForRendering(Transform& transform, Camera& camera, Mesh& mesh, RenderPass renderPass) override;
		void PostRendering() override;

		/** Load the texture for this mesh into OpenGL */
		void SetTexture(const char* texturePath);

		static void ToggleTessellation() { m_useTessellation = !m_useTessellation; }

		/** @return the approximate cost to render the given mesh */
		float GetLODCost(Mesh& mesh) override;
		float GetVertexMultiplier() override { return m_maxInnerTessLevel; }

	private:
		float GetScreenSizeOfMesh(Transform& transform, Camera& camera, Mesh& mesh);

		GLuint m_textureID = -1;
		GLuint m_dispMapID = -1;
		float m_maxInnerTessLevel = 1;
		float m_maxOuterTessLevel = 1;
		float m_displacementMagnitude = 1.0f;
		float m_pixelsPerPolygon = 20;

		static bool m_useTessellation;
	};
}
