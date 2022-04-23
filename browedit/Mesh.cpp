#include "Mesh.h"



void Mesh::init()
{
	std::vector<VertexP3T2N3> verts;
	vertices = buildVertices();
	for (size_t i = 0; i < vertices.size(); i += 3)
	{
		glm::vec3 normal = glm::normalize(glm::cross(vertices[i + 1] - vertices[i], vertices[i + 2] - vertices[i]));
		for (size_t ii = i; ii < i + 3; ii++)
			verts.push_back(VertexP3T2N3(vertices[ii], normal));
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


bool Mesh::collision(const math::Ray& ray, const glm::mat4& modelMatrix)
{
	math::Ray invRay(ray * glm::inverse(modelMatrix));
	float t;
	for (size_t i = 0; i < vertices.size(); i+=3)
		if (invRay.LineIntersectPolygon(std::span<glm::vec3>(vertices.data() + i, 3), t))
			return true;
	return false;
}