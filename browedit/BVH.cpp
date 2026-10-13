#include "BVH.h"
#include <algorithm>

void Bvh::buildTree()
{
	triangleIndices.resize(triangles.size());

	for (uint32_t i = 0; i < triangles.size(); i++)
		triangleIndices[i] = i;

	triangleInfos.clear();
	triangleInfos.resize(triangles.size());

	nodes.clear();
	nodes.reserve(triangles.size() * 2);

	for (int i = 0; i < triangles.size(); i++) {
		auto& triangle = triangles[i];
		auto& triangleInfo = triangleInfos[i];

		triangleInfo.center = (triangle.vertices[0] + triangle.vertices[1] + triangle.vertices[2]) / 3.0f;
		triangleInfo.boundsMin = glm::min(triangle.vertices[0], glm::min(triangle.vertices[1], triangle.vertices[2]));
		triangleInfo.boundsMax = glm::max(triangle.vertices[0], glm::max(triangle.vertices[1], triangle.vertices[2]));
	}

	buildNode(0, (uint32_t)triangles.size());
	triangleInfos.clear();
}

int Bvh::buildNode(uint32_t first, uint32_t count)
{
	BvhNode node;

	glm::vec3 min(FLT_MAX);
	glm::vec3 max(-FLT_MAX);

	for (uint32_t i = 0; i < count; i++)
	{
		const TriangleInfo& triangleInfo = triangleInfos[triangleIndices[first + i]];
		min = glm::min(min, triangleInfo.boundsMin);
		max = glm::max(max, triangleInfo.boundsMax);
	}

	node.aabb.bounds[0] = min;
	node.aabb.bounds[1] = max;

	int nodeIndex = (int)nodes.size();
	nodes.push_back(node);

	if (count <= 8)
	{
		nodes[nodeIndex].firstTriangle = first;
		nodes[nodeIndex].triangleCount = count;
		return nodeIndex;
	}

	glm::vec3 extent = max - min;

	int axis = 0;

	if (extent.y > extent.x)
		axis = 1;

	if (extent.z > extent[axis])
		axis = 2;

	uint32_t middle = first + count / 2;

	std::nth_element(
		triangleIndices.begin() + first,
		triangleIndices.begin() + middle,
		triangleIndices.begin() + first + count,
		[&](uint32_t a, uint32_t b)
		{
			return triangleInfos[a].center[axis] < triangleInfos[b].center[axis];
		});

	uint32_t leftCount = middle - first;
	uint32_t rightCount = count - leftCount;

	int left = buildNode(first, leftCount);
	int right = buildNode(middle, rightCount);

	nodes[nodeIndex].left = left;
	nodes[nodeIndex].right = right;

	return nodeIndex;
}