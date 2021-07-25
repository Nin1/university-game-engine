#include "stdafx.h"
#include "DeferredLightingManager.h"
#include <Core\Application.h>
#include <Components\Transform.h>
#include <algorithm>

namespace snes
{
	const glm::mat4 DeferredLightingManager::BIAS_MATRIX(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0
	);

	DeferredLightingManager::DeferredLightingManager()
	{
	}
	
	DeferredLightingManager::~DeferredLightingManager()
	{
	}

	void DeferredLightingManager::Init()
	{
		m_shader.Load(DEFERRED_LIGHTING_PASS);
		m_shader.SetGlUniformSampler2D("gPosition", 0);
		m_shader.SetGlUniformSampler2D("gNormal", 1);
		m_shader.SetGlUniformSampler2D("gAlbedoSpec", 2);
		m_shader.SetGlUniformSampler2D("gEmissive", 3);
		m_shader.SetGlUniformSampler2D("gShadowMap", 4);

		uint screenWidth, screenHeight;
		Application::GetScreenSize(screenWidth, screenHeight);

		glGenFramebuffers(1, &m_buffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);

		// Set up position buffer
		glGenTextures(1, &m_position);
		glBindTexture(GL_TEXTURE_2D, m_position);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_position, 0);

		// Set up normal buffer
		glGenTextures(1, &m_normal);
		glBindTexture(GL_TEXTURE_2D, m_normal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_normal, 0);

		// Set up albedo/specular buffer (RGB = albedo, A = specular)
		glGenTextures(1, &m_albedoSpecular);
		glBindTexture(GL_TEXTURE_2D, m_albedoSpecular);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_albedoSpecular, 0);

		// Set up render flags buffer (bit 0 = "UseLighting")
		glGenTextures(1, &m_emissive);
		glBindTexture(GL_TEXTURE_2D, m_emissive);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_emissive, 0);

		GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, attachments);

		// Set up depth buffer
		glGenRenderbuffers(1, &m_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth);

		// Check that the framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "Error: Framebuffer Incomplete" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		/* ############################################## */

		// Set up shadow pass buffer
		glGenFramebuffers(1, &m_shadowFBO);
		glGenRenderbuffers(1, &m_shadowRT);
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, m_shadowRT);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_shadowRT);

		// Set up depth texture
		glGenTextures(1, &m_shadowTexture);
		glBindTexture(GL_TEXTURE_2D, m_shadowTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowTexture, 0);

		// Check that the framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "Error: Shadow Buffer Incomplete" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void DeferredLightingManager::PrepareNewGeometryPass()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_buffer);
		uint screenWidth, screenHeight;
		Application::GetScreenSize(screenWidth, screenHeight);
		glViewport(0, 0, screenWidth, screenHeight);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void DeferredLightingManager::PrepareNewShadowPass()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
		glViewport(0, 0, 2048, 2048);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	void DeferredLightingManager::RenderLighting(std::shared_ptr<Camera> camera, std::vector<std::weak_ptr<PointLight>> pointLights, std::shared_ptr<DirectionalLight> directionalLight)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		uint screenWidth, screenHeight;
		Application::GetScreenSize(screenWidth, screenHeight);
		glViewport(0, 0, screenWidth, screenHeight);
		glUseProgram(m_shader.GetProgramID()); 

		if (pointLights.size() > MAX_POINT_LIGHTS)
		{
			std::cout << "Warning: Too many point lights in scene. Not all will be used." << std::endl;
			std::cout << "Maximum point lights: " << MAX_POINT_LIGHTS << std::endl;
			std::cout << "Point lights in scene: " << pointLights.size() << std::endl;
		}
		uint numPointLights = std::min((uint)pointLights.size(), MAX_POINT_LIGHTS);

		// Send point lights to shader
		for (uint i = 0; i < numPointLights; ++i)
		{
			auto pointlight = pointLights[i].lock();
			if (pointlight)
			{
				m_shader.SetGlUniformVec3(("pointLights[" + std::to_string(i) + "].pos").c_str(), pointlight->GetTransform().GetWorldPosition());
				m_shader.SetGlUniformVec3(("pointLights[" + std::to_string(i) + "].colour").c_str(), pointlight->GetColour());
				m_shader.SetGlUniformFloat(("pointLights[" + std::to_string(i) + "].linear").c_str(), pointlight->GetLinearAttenuation());
				m_shader.SetGlUniformFloat(("pointLights[" + std::to_string(i) + "].quadratic").c_str(), pointlight->GetQuadraticAttenuation());
			}
		}

		m_shader.SetGlUniformBool("normalsOnly", m_normalsOnly);
		m_shader.SetGlUniformBool("shadowsOnly", m_shadowsOnly);
		m_shader.SetGlUniformVec3("viewPos", camera->GetTransform().GetWorldPosition());
		m_shader.SetGlUniformMat4("cameraViewMat", camera->GetViewMatrix());
		m_shader.SetGlUniformMat4("shadowVPMat", directionalLight->GetViewProjectionMatrix());
		m_shader.SetGlUniformVec3("eyeSpaceLightPos", camera->GetViewMatrix() * glm::vec4(directionalLight->GetTransform().GetWorldPosition(), 1.0f));
		m_shader.SetGlUniformVec3("directionalLightColour", directionalLight->GetColour());
		m_shader.SetGlUniformVec3("directionalLightEyeDirection", camera->GetViewMatrix() * glm::vec4(directionalLight->GetTransform().GetWorldRotation(), 0.0f));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_position);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_normal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_albedoSpecular);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, m_emissive);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, m_shadowTexture);

		RenderQuad();
	}

	void DeferredLightingManager::RenderQuad()
	{
		if (m_quadVAO == 0)
		{
			float quadVertices[] {
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};

			glGenVertexArrays(1, &m_quadVAO);
			glGenBuffers(1, &m_quadVBO);
			glBindVertexArray(m_quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(m_quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}
}
