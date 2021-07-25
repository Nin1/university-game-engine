#pragma once
#include "GameObject.h"
#include <Components\Camera.h>
#include <Rendering\DeferredLightingManager.h>
#include <Rendering\Material.h>

namespace snes
{
	/** Scene
	  * Sets up a demo scene and handles the flow of logic between GameObjects*/
	class Scene
	{
	public:
		Scene();
		~Scene();

		void InitialiseScene();

		void FixedLogic();
		void MainLogic() { m_root->MainLogic(); }
		void MainDraw();
		
	private:
		GameObject& CreateJiggy(glm::vec3 pos, std::shared_ptr<Camera> camera, std::shared_ptr<Material> material);
		std::weak_ptr<GameObject> CreateLink(glm::vec3 pos, std::shared_ptr<Camera> camera);
		GameObject& CreateSphere(glm::vec3 pos, std::shared_ptr<Camera> camera, std::weak_ptr<GameObject> lodReferenceObj);
		GameObject& CreateFloor(std::shared_ptr<Camera> camera, glm::vec3 pos);
		GameObject& CreatePointLight(GameObject& parent, glm::vec3 pos, glm::vec3 colour);
		GameObject& CreateBillboard(glm::vec3 pos, std::shared_ptr<Camera> camera);
		GameObject& CreateObject(std::string modelPath, std::shared_ptr<Camera> camera, glm::vec3 pos, glm::vec3 scale = glm::vec3(1.0f), glm::vec3 rot = glm::vec3(0.0f));

		std::shared_ptr<GameObject> m_root;
		std::shared_ptr<GameObject> m_camera;
		std::shared_ptr<GameObject> m_directionalLight;

		std::vector<std::weak_ptr<PointLight>> m_pointLights;

		GLuint m_queryID;

		DeferredLightingManager m_deferredLightingMgr;
	};
}