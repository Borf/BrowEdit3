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

	bool Ray::LineIntersectPolygon(const std::span<glm::vec3> &vertices, float &t) const
	{
		// Möller–Trumbore intersection algorithm
		constexpr float epsilon = 0.001f;

		glm::vec3 edge1 = vertices[1] - vertices[0];
		glm::vec3 edge2 = vertices[2] - vertices[0];
		glm::vec3 ray_cross_e2 = glm::cross(dir, edge2);
		float det = glm::dot(edge1, ray_cross_e2);

		if (det > -epsilon && det < epsilon)
			return false;

		float inv_det = 1.0f / det;
		glm::vec3 s = origin - vertices[0];
		float u = inv_det * glm::dot(s, ray_cross_e2);

		if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1) > epsilon))
			return false;

		glm::vec3 s_cross_e1 = glm::cross(s, edge1);
		float v = inv_det * glm::dot(dir, s_cross_e1);

		if ((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1) > epsilon))
			return false;

		float tt = inv_det * glm::dot(edge2, s_cross_e1);

		if (tt > epsilon) {
			t = tt;
			return true;
		}

		return false;
	}




	Ray Ray::operator*(const glm::mat4 &matrix) const
	{
		glm::vec3 p1 = glm::vec3(matrix * glm::vec4(origin, 1));
		glm::vec3 p2 = glm::vec3(matrix * glm::vec4(origin+dir, 1));
		return Ray(p1, glm::normalize(p2-p1));
	}

}
