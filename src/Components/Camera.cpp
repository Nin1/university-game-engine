#include "stdafx.h"
#include "Camera.h"
#include <Core\Application.h>
#include <Core\FrameTime.h>
#include <Core\GameObject.h>
#include <Core\Input.h>

namespace snes
{
	void Camera::CalculateCurrentProjMatrix()
	{
		if (!m_orthographic)
		{
			auto fov = glm::radians(m_fieldOfView);
			uint scrWidth = 0;
			uint scrHeight = 0;
			Application::GetScreenSize(scrWidth, scrHeight);
			m_projMatrix = glm::perspective(fov, (float)scrWidth / (float)scrHeight, m_nearClipPlane, m_farClipPlane);
		}
		else
		{
			m_projMatrix = glm::ortho(-650.0f, 650.0f, -350.0f, 300.0f, -0.0f, 1000.0f);
		}
	}

	void Camera::CalculateCurrentViewMatrix()
	{
		const glm::vec3& cameraPosition = m_transform.GetWorldPosition();

		m_viewMatrix = glm::lookAt(
			cameraPosition,						// Camera position
			cameraPosition + GetCameraDirection(),	// Camera "look-at" point
			glm::vec3(0, 1, 0)					// Up vector
		);
	}

	glm::vec3 Camera::GetCameraDirection()
	{
		glm::vec3 cameraDirection;
		const glm::vec3& rot = m_transform.GetWorldRotationRadians();
		cameraDirection.x = cos(rot.x) * cos(rot.y);
		cameraDirection.y = sin(rot.x);
		cameraDirection.z = cos(rot.x) * sin(rot.y);
		return cameraDirection;
	}

	void Camera::MainLogic()
	{
		/** Calculate the view/proj matrices this frame */
		CalculateCurrentProjMatrix();
		CalculateCurrentViewMatrix();
		m_projViewMatrix = GetProjMatrix() * GetViewMatrix();
	}

	float Camera::GetVerticalFoV()
	{
		return (m_fieldOfView / 4) * 3; // 4:3
	}
}