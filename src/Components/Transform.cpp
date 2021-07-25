#include "stdafx.h"
#include "Transform.h"
#include <Core\GameObject.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace snes
{
	Transform::Transform(GameObject& gameObject)
		: m_gameObject(gameObject)
	{
		m_localScale = glm::vec3(1.0f);
		m_localRotation = glm::vec3(0.0f);
		m_localPosition = glm::vec3(0.0f);
	}
	
	Transform::~Transform()
	{
	}

	void Transform::Rotate(glm::vec3 rotation)
	{
		m_localRotation += rotation; m_dirtyTRS = true;
	}

	glm::mat4 Transform::GetTRS()
	{
		if (m_dirtyTRS)
		{
			glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_localScale);
			glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_localPosition);
			glm::vec3 eulerAngles = m_localRotation / (180.0f / 3.14159f);
			glm::mat4 eulerRotation = glm::eulerAngleYXZ(eulerAngles.y, eulerAngles.x, eulerAngles.z);

			auto parent = m_gameObject.GetParent();
			if (parent)
			{
				m_trs = parent->GetTransform().GetTRS() * translate * eulerRotation * scale;
			}
			else
			{
				m_trs = translate * eulerRotation * scale;
			}
			m_dirtyTRS = false;
		}
		return m_trs;
	}

	glm::vec3 Transform::GetWorldPosition()
	{
		// Get position in relation to translate/rotate/scale of parent(s)
		return glm::vec3(GetTRS() * glm::vec4(1.0f));
	}

	glm::vec3 Transform::GetWorldRotation() const
	{
		glm::vec3 rot = m_localRotation;
		auto parent = m_gameObject.GetParent();
		if (parent)
		{
			rot += parent->GetTransform().GetWorldRotation();
		}
		return rot;
	}

	glm::vec3 Transform::GetWorldRotationRadians() const
	{
		return GetWorldRotation() / (180.0f / 3.14159f);
	}

	glm::vec3 Transform::GetWorldScale() const
	{
		glm::vec3 scale = m_localScale;
		auto parent = m_gameObject.GetParent();
		if (parent)
		{
			scale *= parent->GetTransform().GetWorldScale();
		}
		return scale;
	}

	glm::vec3 Transform::GetForwardVector() const
	{
		glm::vec3 forward;
		const glm::vec3& rot = GetWorldRotationRadians();
		forward.x = cos(rot.x) * cos(rot.y);
		forward.y = sin(rot.x);
		forward.z = cos(rot.x) * sin(rot.y);
		return forward;
	}
}
