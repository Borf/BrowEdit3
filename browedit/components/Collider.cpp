#include "Collider.h"

std::vector<glm::vec3> Collider::getCollisions(const math::Ray& ray) {
	std::vector<glm::vec3> ret;
	isColliding(ray, ret);
	return ret;
}