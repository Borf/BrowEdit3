#include "Gadget.h"
#include <browedit/math/AABB.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

void Gadget::init()
{
	shader = new SimpleShader();
	shader->use();
	shader->setUniform(SimpleShader::Uniforms::s_texture, 0);

	arrowMesh.init();

}

void Gadget::setMatrices(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
{
	Gadget::projectionMatrix = projectionMatrix;
	Gadget::viewMatrix = viewMatrix;
}

void Gadget::draw(const math::Ray& mouseRay, const glm::mat4& modelMatrix)
{
	axisClicked = false;
	axisReleased = false;

	shader->use();
	shader->setUniform(SimpleShader::Uniforms::projectionMatrix, projectionMatrix);
	shader->setUniform(SimpleShader::Uniforms::viewMatrix, viewMatrix);
	shader->setUniform(SimpleShader::Uniforms::modelMatrix, modelMatrix);
	shader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);


	auto drawDirection = [&](const glm::mat4& matrix, int index, const glm::vec4& color)
	{
		shader->setUniform(SimpleShader::Uniforms::modelMatrix, matrix);
		if (arrowMesh.collision(mouseRay, matrix) || selectedAxis == index)
		{
			if(selectedAxis == 0)
				shader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 0.25f, 1));
			else
				shader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1.5, 1.5, 0.65f, 1));
			if (ImGui::IsMouseDown(0) && selectedAxis == 0)
			{
				axisDragged = true;
				axisClicked = true;
				selectedAxis = index;
			}
		}
		else
			shader->setUniform(SimpleShader::Uniforms::color, color);
		arrowMesh.draw();
		if (!ImGui::IsMouseDown(0) && selectedAxis == index)
		{
			//mouseup
			selectedAxis = 0;
			axisDragged = false;
			axisReleased = true;
		}

	};


	drawDirection(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0, 0, -1)), Axis::X, glm::vec4(1, 0.25f, 0.25f, 1));
	drawDirection(modelMatrix, Axis::Y, glm::vec4(0.25f, 1, 0.25f, 1));
	drawDirection(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0)), Axis::Z, glm::vec4(0.25f, 0.25f, 1, 1));


}



int Gadget::selectedAxisIndex()
{
	if (selectedAxis == Axis::X)
		return 0;
	if (selectedAxis == Axis::Y)
		return 1;
	if (selectedAxis == Axis::Z)
		return 2;

	return 0;
}



std::vector<glm::vec3> Gadget::ArrowMesh::buildVertices()
{
	int height = 20;
	int tip = 30;

	std::vector<glm::vec3> verts = math::AABB::box(glm::vec3(-2, 5, -2), glm::vec3(2, height, 2));
	verts.push_back(glm::vec3(-5, height, -5));
	verts.push_back(glm::vec3(-5, height, 5));
	verts.push_back(glm::vec3(0, tip, 0));

	verts.push_back(glm::vec3(-5, height, 5));
	verts.push_back(glm::vec3(5, height, 5));
	verts.push_back(glm::vec3(0, tip, 0));

	verts.push_back(glm::vec3(5, height, 5));
	verts.push_back(glm::vec3(5, height, -5));
	verts.push_back(glm::vec3(0, tip, 0));

	verts.push_back(glm::vec3(5, height, -5));
	verts.push_back(glm::vec3(-5, height, -5));
	verts.push_back(glm::vec3(0, tip, 0));
	return verts;
}
