#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <browedit/gl/Vertex.h>
#include <browedit/gl/VBO.h>
#include <browedit/math/Ray.h>

class Mesh
{
	std::vector<glm::vec3> vertices; //for collision testing

public:
	gl::VBO<VertexP3T2N3>* vbo = nullptr;

	void init(bool calculateNormals = true);
	void draw();
	bool collision(const math::Ray& ray, const glm::mat4& modelMatrix, float& t);
	glm::vec3 getCollision(const math::Ray& ray, const glm::mat4& modelMatrix);


	void addVertex(const glm::vec3& pos, const glm::vec2& tex = glm::vec2(0, 0));


	virtual void buildVertices(std::vector<VertexP3T2N3> &verts) = 0;
};