#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <browedit/gl/Vertex.h>
#include <browedit/gl/VBO.h>
#include <browedit/math/Ray.h>

class Mesh
{
	
public:
	void init();
	void draw();
	bool collision(const math::Ray& ray, const glm::mat4& modelMatrix);

	std::vector<glm::vec3> vertices;
	gl::VBO<VertexP3T2N3>* vbo = nullptr;

	virtual std::vector<glm::vec3> buildVertices() = 0;
};