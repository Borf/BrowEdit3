#pragma once

#include "Component.h"
#include <vector>
#include <glm/glm.hpp>
namespace math { class Ray; }

class Collider : public Component
{
public:
	virtual std::vector<glm::vec3> getCollisions(const math::Ray &ray) = 0;
};