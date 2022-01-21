#pragma once

#include <glm/glm.hpp>

namespace math
{

	class Plane
	{
	public:
		Plane()
		{
			D = 0;
		}

		Plane(const glm::vec3 &normal, float d)
		{
			this->normal = normal;
			this->D = d;
		}

		Plane(const glm::vec3& normal, const glm::vec3 &positionOnPlane)
		{
			this->normal = normal;
			this->D = -glm::dot(positionOnPlane, normal);
		}


		glm::vec3 normal;
		float D;
	};
}
