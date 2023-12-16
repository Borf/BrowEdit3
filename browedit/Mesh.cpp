#include "Mesh.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

void Mesh::init(bool calculateNormals)
{
	std::vector<VertexP3T2N3> verts;
	vertices.clear();
	buildVertices(verts);

	if (calculateNormals) {
		for (size_t i = 0; i < verts.size(); i += 3)
		{
			glm::vec3 normal = glm::normalize(glm::cross(glm::make_vec3(verts[i + 1].data) - glm::make_vec3(verts[i].data), glm::make_vec3(verts[i + 2].data) - glm::make_vec3(verts[i].data)));

			for (size_t ii = i; ii < i + 3; ii++)
			{
				verts[ii].data[3 + 2 + 0] = normal.x;
				verts[ii].data[3 + 2 + 1] = normal.y;
				verts[ii].data[3 + 2 + 2] = normal.z;
				vertices.push_back(glm::make_vec3(verts[ii].data));
			}
		}
	}
	else {
		for (size_t i = 0; i < verts.size(); i += 3)
		{
			for (size_t ii = i; ii < i + 3; ii++)
			{
				vertices.push_back(glm::make_vec3(verts[ii].data));
			}
		}
	}

	if(!vbo)
		vbo = new gl::VBO<VertexP3T2N3>();
	vbo->setData(verts, GL_STATIC_DRAW);
}

void Mesh::draw()
{
	vbo->bind();
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4); //TODO: vao

	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), (void*)(0 * sizeof(float)));
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), (void*)(3 * sizeof(float)));
	glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), (void*)(5 * sizeof(float)));

	glDrawArrays(GL_TRIANGLES, 0, (int)vbo->size());
	vbo->unBind();
}


bool Mesh::collision(const math::Ray& ray, const glm::mat4& modelMatrix, float& t)
{
	math::Ray invRay(ray * glm::inverse(modelMatrix));
	for (size_t i = 0; i < vertices.size(); i+=3)
		if (invRay.LineIntersectPolygon(std::span<glm::vec3>(vertices.data() + i, 3), t))
			return true;
	return false;
}

glm::vec3 Mesh::getCollision(const math::Ray& ray, const glm::mat4& modelMatrix)
{
	math::Ray invRay(ray * glm::inverse(modelMatrix));
	float t;
	std::vector<glm::vec3> points;
	for (size_t i = 0; i < vertices.size(); i += 3)
		if (invRay.LineIntersectPolygon(std::span<glm::vec3>(vertices.data() + i, 3), t))
			points.push_back(invRay.origin + t * invRay.dir);

	std::sort(points.begin(), points.end(), [&](const glm::vec3& a, const glm::vec3& b) { return glm::distance(a, invRay.origin) < glm::distance(b, invRay.origin); });
	return points[0];
}

