#pragma once
#include "..\Material.h"

namespace snes
{
	/** Disco Material
	  * A material that changes hue over time */
	class DiscoMat : public Material
	{
	public:
		DiscoMat();
		DiscoMat(std::ifstream& params);
		~DiscoMat();

		void PrepareForRendering(RenderPass renderPass) override;

		/** @return the approximate cost to render the given mesh */
		float GetLODCost(Mesh& mesh) override;
	private:
		float m_hue;
	};
}
