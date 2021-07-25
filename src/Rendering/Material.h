#pragma once
#include "ShaderProgram.h"
#include <Components\Transform.h>
#include <utility>

namespace snes
{
	class Camera;
	class Transform;
	class Mesh;

	// Derive from this to make your material
	class Material
	{
	public:
		~Material();

		/** Called before rendering, set uniforms in here */
		virtual void PrepareForRendering(RenderPass renderPass);
		virtual void PrepareForRendering(Transform& transform, Camera& camera, Mesh& mesh, RenderPass renderPass) { this->PrepareForRendering(renderPass); }
		virtual void PostRendering() {};

		/** Set the model, view, and projection uniforms (these must be present in every material shader) */
		virtual void ApplyTransformUniforms(glm::mat4& model, glm::mat4& view, glm::mat4& proj, glm::mat4& projView);

		bool GetUsePatches() { return m_usePatches; }
		virtual bool RequiresNeighbourData() { return false; }

		virtual float GetLODCost(Mesh& mesh) = 0;
		virtual float GetBenefit() { return 1.0f; }
		virtual float GetVertexMultiplier() { return 1.0f; }

	public:
		static std::shared_ptr<Material> CreateMaterial(const char* matPath);
		static std::shared_ptr<Material> CreateMaterial(std::ifstream& params);
		static std::shared_ptr<Material> CreateShadowMaterial(const char* matPath);
		static void ResetCurrentShader() { m_currentShader = NONE; }
		static GLuint GetTexture(std::string texturePath);

	protected:
		Material(ShaderName shaderName);

		/** Set a uniform of the given type with the given name to the given value */
		void SetUniformMat4(const char* name, glm::mat4 value);
		void SetUniformVec2(const char* name, glm::vec2 value);
		void SetUniformVec3(const char* name, glm::vec3 value);
		void SetUniformFloat(const char* name, float value);
		void SetUniformSampler2D(const char* name, GLuint value);
		void SetUniformBool(const char* name, bool value);

		bool m_usePatches = false;
		GLuint m_queryID = -1;

	protected:
		/** Cache of all loaded shaders */
		static std::map<ShaderName, std::weak_ptr<ShaderProgram>> m_shaders;
		/** Cache of all loaded textures */
		static std::map<std::string, GLuint> m_loadedTextures;

	private:
		ShaderName m_shaderName; 

		std::shared_ptr<ShaderProgram> m_shader;

		std::vector<std::pair<std::string, glm::vec3>> m_vec3s;
		std::vector<std::pair<std::string, glm::vec2>> m_vec2s;
		std::vector<std::pair<std::string, glm::mat4>> m_mat4s;
		std::vector<std::pair<std::string, float>> m_floats;
		std::vector<std::pair<std::string, GLuint>> m_sampler2Ds;
		std::vector<std::pair<std::string, bool>> m_bools;

		static ShaderName m_currentShader;
	};
}
