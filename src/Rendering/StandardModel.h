#pragma once
#include <Core\Component.h>
#include <Rendering\Mesh.h>
#include <Rendering\Material.h>
#include <Rendering\RenderModel.h>

namespace snes
{
	class Camera;

	class StandardModel : public RenderModel
	{
	public:
		StandardModel() : RenderModel() { };
		~StandardModel() { };

		/** Load mesh & material data */
		void Load(std::string fileName) override;
		void Render(RenderPass renderPass, Camera& camera, Transform& transform) override;
		float CalculateLODValue(float meshSize, const Mesh& baseMesh) override;
		float CalculateCost() override;

		virtual const std::weak_ptr<Mesh> GetMesh() override { return m_mesh; }
				
	private:
		GLuint m_queryID = -1;

	private:
		/** Calculate the model/view/proj matrices and apply them to the material */
		void PrepareTransformUniforms(Camera& camera, Material* mat, Transform& transform);
		
		/**  */
		std::shared_ptr<Mesh> m_mesh;
		std::shared_ptr<Material> m_material;
		int m_pixelsRenderedLastFrame = 0;
		//std::shared_ptr<Material> m_shadowMaterial;
	};
}
