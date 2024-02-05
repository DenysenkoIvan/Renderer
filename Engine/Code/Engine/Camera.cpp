#include "Camera.h"

#include <Framework/Common.h>

const glm::vec3& Camera::GetPosition() const
{
	return m_position;
}

const glm::vec3& Camera::GetFront() const
{
	return m_front;
}

float Camera::GetAspectRatio() const
{
	return m_aspectRatio;
}

float Camera::GetNear() const
{
	return m_near;
}

float Camera::GetFar() const
{
	return m_far;
}

void Camera::SetPosition(const glm::vec3& position)
{
	m_position = position;
}

void Camera::SetAspectRatio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;
}

void Camera::SetNear(float nearDistance)
{
	m_near = nearDistance;
}

void Camera::SetFar(float farDistance)
{
	m_far = farDistance;
}

void Camera::MoveForward(float movement)
{
	m_position += m_front * movement;
}

void Camera::MoveLeft(float movement)
{
	glm::vec3 left = glm::normalize(glm::cross(m_up, m_front));
	m_position += left * movement;
}

void Camera::MoveUp(float movement)
{
	m_position += m_up * movement;
}

void Camera::TurnLeft(float degrees)
{
	glm::mat4 m = glm::rotate(glm::mat4(1.0f), glm::radians(degrees), m_up);
	m_front = m * glm::vec4(m_front, 1.0f);
	m_front = glm::normalize(m_front);
}

void Camera::TurnUp(float degrees)
{
	float newVerticalAngle = std::clamp(m_verticalAngle + degrees, -89.0f, 89.0f);

	float newDegrees = newVerticalAngle - m_verticalAngle;
	m_verticalAngle = newVerticalAngle;

	glm::mat4 m = glm::rotate(glm::mat4(1.0f), glm::radians(newDegrees), glm::cross(m_up, m_front));
	m_front = m * glm::vec4(m_front, 1.0f);
	m_front = glm::normalize(m_front);
}

glm::mat4 Camera::ViewMatrix() const
{
	return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::ProjMatrix() const
{
	glm::mat4 proj = glm::perspective(glm::radians(80.0f), m_aspectRatio, m_near, m_far);

	return proj;
}