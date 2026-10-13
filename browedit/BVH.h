#pragma once

#include <glm/glm.hpp>
#include <browedit\math\AABB.h>
#include <vector>

struct Triangle {
	glm::vec3 vertices[3];
	glm::vec2 uvs[3];
	glm::vec3 normal;
	int texId;
	float shadowStrength;
};

struct BvhNode
{
	int left = -1;
	int right = -1;

	uint32_t firstTriangle = -1;
	int triangleCount = 0;

	bool isLeaf() const
	{
		return triangleCount != 0;
	}

	math::AABB aabb = math::AABB(glm::vec3(0), glm::vec3(0));
};

class Bvh
{
public:
	std::vector<Triangle> triangles;
	std::vector<uint32_t> triangleIndices;
	std::vector<BvhNode> nodes;

	void buildTree();
private:
	struct TriangleInfo {
		glm::vec3 center;
		glm::vec3 boundsMin;
		glm::vec3 boundsMax;
	};

	std::vector<TriangleInfo> triangleInfos;
	int buildNode(uint32_t first, uint32_t count);
};