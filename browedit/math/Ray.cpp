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
		Plane plane;
		plane.normal = glm::cross(vertices[1] - vertices[0], vertices[2] - vertices[0]);
		if (glm::length(plane.normal) < 0.000001f)
			return false;
		plane.normal = glm::normalize(plane.normal);
		plane.D = -glm::dot(plane.normal, vertices[0]);
		float tt;

		if (!planeIntersection(plane, tt))
			return false;

		glm::vec3 intersection = origin + dir * tt;

		/*	if (Intersection == EndLine)
		return false;*/
		for (size_t vertex = 0; vertex < vertices.size(); vertex++)
		{
			Plane edgePlane;
			int NextVertex = (vertex + 1) % (int)vertices.size();
			glm::vec3 EdgeVector = vertices[NextVertex] - vertices[vertex];
			edgePlane.normal = glm::normalize(glm::cross(EdgeVector, plane.normal));
			edgePlane.D = -glm::dot(edgePlane.normal, vertices[vertex]);

			if (glm::dot(edgePlane.normal, intersection) + edgePlane.D > 0.001f)
				return false;
		}

		t = tt;
		return true;
	}




	Ray Ray::operator*(const glm::mat4 &matrix) const
	{
		glm::vec3 p1 = glm::vec3(matrix * glm::vec4(origin, 1));
		glm::vec3 p2 = glm::vec3(matrix * glm::vec4(origin+dir, 1));
		return Ray(p1, glm::normalize(p2-p1));
	}

}
