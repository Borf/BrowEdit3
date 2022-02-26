#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <span>

namespace math
{
	class Ray;
	class AABB
	{
	public:
		glm::vec3 bounds[2];
		glm::vec3 &min;
		glm::vec3 &max;

		AABB(const glm::vec3& min, const glm::vec3& max);
		AABB(const std::span<glm::vec3> &verts);
		bool hasRayCollision(const Ray& r, float minDistance, float maxDistance) const;

		static std::vector<glm::vec3> box(const glm::vec3& tl, const glm::vec3& br);
		static std::vector<glm::vec3> boxVerts(const glm::vec3& tl, const glm::vec3& br);
	};
}