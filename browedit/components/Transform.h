#pragma once

#include "Component.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform : public Component
{
public:
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	glm::mat4 buildMatrix();

	~Transform() {}
};