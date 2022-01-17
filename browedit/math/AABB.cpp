#include "AABB.h"
#include "Ray.h"

math::AABB::AABB(const glm::vec3 &min, const glm::vec3 &max) : min(bounds[0]), max(bounds[1])
{
	bounds[0] = min;
	bounds[1] = max;
}


/*
Check if a ray has collision with this boundingbox. Thanks to http://www.cs.utah.edu/~awilliam/box/box.pdf
Author: Bas Rops - 09-06-2014
*/
bool math::AABB::hasRayCollision(const Ray& r, float minDistance, float maxDistance) const
{
	float tmin, tmax, tymin, tymax, tzmin, tzmax;

	tmin = (bounds[r.sign[0]].x - r.origin.x) * r.invDir.x;
	tmax = (bounds[1 - r.sign[0]].x - r.origin.x) * r.invDir.x;

	tymin = (bounds[r.sign[1]].y - r.origin.y) * r.invDir.y;
	tymax = (bounds[1 - r.sign[1]].y - r.origin.y) * r.invDir.y;

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	tzmin = (bounds[r.sign[2]].z - r.origin.z) * r.invDir.z;
	tzmax = (bounds[1 - r.sign[2]].z - r.origin.z) * r.invDir.z;

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;

	if (tzmin > tmin)
		tmin = tzmin;

	if (tzmax < tmax)
		tmax = tzmax;

	return ((tmin < maxDistance) && (tmax > minDistance));
}


std::vector<glm::vec3> math::AABB::box(const glm::vec3& tl, const glm::vec3& br)
{
	std::vector<glm::vec3> ret;
	//front
	ret.push_back(glm::vec3(tl.x, br.y, tl.z));
	ret.push_back(glm::vec3(br.x, tl.y, tl.z));
	ret.push_back(glm::vec3(tl.x, tl.y, tl.z));

	ret.push_back(glm::vec3(br.x, br.y, tl.z));
	ret.push_back(glm::vec3(br.x, tl.y, tl.z));
	ret.push_back(glm::vec3(tl.x, br.y, tl.z));
	//back
	ret.push_back(glm::vec3(tl.x, tl.y, br.z));
	ret.push_back(glm::vec3(br.x, tl.y, br.z));
	ret.push_back(glm::vec3(tl.x, br.y, br.z));

	ret.push_back(glm::vec3(tl.x, br.y, br.z));
	ret.push_back(glm::vec3(br.x, tl.y, br.z));
	ret.push_back(glm::vec3(br.x, br.y, br.z));

	//top
	ret.push_back(glm::vec3(tl.x, tl.y, tl.z));
	ret.push_back(glm::vec3(br.x, tl.y, tl.z));
	ret.push_back(glm::vec3(tl.x, tl.y, br.z));

	ret.push_back(glm::vec3(br.x, tl.y, br.z));
	ret.push_back(glm::vec3(br.x, tl.y, tl.z));
	ret.push_back(glm::vec3(tl.x, tl.y, br.z));


	//bottom
	ret.push_back(glm::vec3(tl.x, br.y, br.z));
	ret.push_back(glm::vec3(br.x, br.y, tl.z));
	ret.push_back(glm::vec3(tl.x, br.y, tl.z));

	ret.push_back(glm::vec3(br.x, br.y, br.z));
	ret.push_back(glm::vec3(br.x, br.y, tl.z));
	ret.push_back(glm::vec3(tl.x, br.y, br.z));

	//left
	ret.push_back(glm::vec3(tl.x, tl.y, br.z));
	ret.push_back(glm::vec3(tl.x, br.y, tl.z));
	ret.push_back(glm::vec3(tl.x, tl.y, tl.z));

	ret.push_back(glm::vec3(tl.x, br.y, br.z));
	ret.push_back(glm::vec3(tl.x, br.y, tl.z));
	ret.push_back(glm::vec3(tl.x, tl.y, br.z));
	//right
	ret.push_back(glm::vec3(br.x, tl.y, tl.z));
	ret.push_back(glm::vec3(br.x, br.y, tl.z));
	ret.push_back(glm::vec3(br.x, tl.y, br.z));

	ret.push_back(glm::vec3(br.x, tl.y, br.z));
	ret.push_back(glm::vec3(br.x, br.y, tl.z));
	ret.push_back(glm::vec3(br.x, br.y, br.z));

	return ret;
}