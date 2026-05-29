#include "Ray.h"
#include "Plane.h"

namespace math
{

	Ray::Ray(glm::vec3 origin, glm::vec3 dir)
	{
		this->origin = origin;
		this->dir = dir;

		calcSign();
	}

	void Ray::calcSign()
	{
		invDir = 1.0f / dir;// glm::vec3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
		sign[0] = (invDir.x < 0);
		sign[1] = (invDir.y < 0);
		sign[2] = (invDir.z < 0);
	}


	bool Ray::planeIntersection(const Plane &plane, float &t) const
	{
		float Denominator = glm::dot(dir, plane.normal);
		if (fabs(Denominator) <= 0.001f) // Parallel to the plane
		{
			return false;
		}
		float Numerator = glm::dot(origin, plane.normal) + plane.D;
		t = -Numerator / Denominator;
/*			if (t < 0.0f || t > 1.0f) // The intersection point is not on the line
		{
			return false;
		}*/
		return true;
	}

	bool Ray::LineIntersectPolygon(const std::span<glm::vec3> &vertices, float &t, const float epsilon) const
	{
