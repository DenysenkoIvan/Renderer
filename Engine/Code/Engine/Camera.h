#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	const glm::vec3& GetPosition() const;
	const glm::vec3& GetFront() const;
	float GetAspectRatio() const;
	float GetNear() const;
	float GetFar() const;

	void SetPosition(const glm::vec3& position);
	void SetAspectRatio(float aspectRatio);
	void SetNear(float nearDistance);
	void SetFar(float farDistance);

	void MoveForward(float movement);
	void MoveLeft(float movement);
	void MoveUp(float movement);

	void TurnLeft(float degrees);
	void TurnUp(float degrees);

	glm::mat4 ViewMatrix() const;
	glm::mat4 ProjMatrix() const;

private:
	glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 m_front = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 m_up = glm::vec3(0.0f, 0.0f, 1.0f);
	float m_verticalAngle = 0.0f;

	float m_aspectRatio = 1.0f;
	float m_near = 0.1f;
	float m_far = 1024.0f;
};