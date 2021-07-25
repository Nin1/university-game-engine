#pragma once
#include <Core\Component.h>
#include <Rendering\Mesh.h>

namespace snes
{
	class RenderModel
	{
	public:
		RenderModel() {};
		~RenderModel() {};

		virtual void Render(RenderPass renderPass, Camera& camera, Transform& transform) = 0;

		/** Load mesh with data at "filename.mdl" */
		virtual void Load(std::string fileName) = 0;
		virtual float CalculateLODValue(float meshSize, const Mesh& baseMesh) = 0;
		virtual float CalculateCost() = 0;
		virtual const std::weak_ptr<Mesh> GetMesh() = 0;
	};
}
