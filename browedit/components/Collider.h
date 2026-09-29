#pragma once

#include "Component.h"
#include <vector>
#include <glm/glm.hpp>
namespace math { class Ray; }

class Collider : public Component
{
public:
	virtual bool isColliding(const math::Ray& ray, std::vector<glm::vec3>& ret) = 0;
	std::vector<glm::vec3> getCollisions(const math::Ray& ray);
};
