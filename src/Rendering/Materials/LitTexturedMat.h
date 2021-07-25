#pragma once
#include "..\Material.h"
#include <fstream>

namespace snes
{
	/** Unlit Textured Material
	  * Renders a mesh with a texture.
	  * Perhaps incorrectly named, since the deferred lighting pipeline causes these to be lit quite nicely now :) */
	class LitTexturedMat : public Material
	{
	public:
		LitTexturedMat();
		LitTexturedMat(std::ifstream& params);
		~LitTexturedMat();
		
		void PrepareForRendering(RenderPass renderPass) override;

		/** Load the texture for this mesh into OpenGL */
		void SetTexture(const char* texturePath);

		/** @return the approximate cost to render the given mesh */
		float GetLODCost(Mesh& mesh) override;
	private:
		GLuint m_textureID = -1;
	};
}
