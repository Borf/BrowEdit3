#pragma once

#include <glm/glm.hpp>

namespace math
{
	class Ray;
	class AABB
	{
	public:
		glm::vec3 bounds[2];
		glm::vec3 &min;
		glm::vec3 &max;

		AABB(const glm::vec3 &min, const glm::vec3 &max);
		bool hasRayCollision(const Ray& r, float minDistance, float maxDistance) const;
	};
}