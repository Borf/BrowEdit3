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
	rotateMesh.init();
	scaleMesh.init();
	scaleCubeMesh.init();
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
	static bool axisHovered;
	axisHovered = false;

	shader->use();
	shader->setUniform(SimpleShader::Uniforms::projectionMatrix, projectionMatrix);
	shader->setUniform(SimpleShader::Uniforms::viewMatrix, viewMatrix);
	shader->setUniform(SimpleShader::Uniforms::modelMatrix, modelMatrix);
	shader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);


	auto drawDirection = [&](const glm::mat4& matrix, int index, const glm::vec4& color)
	{
		Mesh* mesh = nullptr;
		if (mode == Mode::Translate)	mesh = &arrowMesh;
		if (mode == Mode::TranslateY)	mesh = &arrowMesh;
		if (mode == Mode::Rotate)		mesh = &rotateMesh;
		if (mode == Mode::Scale)		mesh = &scaleMesh;
		if (mode == Mode::Scale && index != 0 && (index & (index - 1)) != 0)		mesh = &scaleCubeMesh;

		shader->setUniform(SimpleShader::Uniforms::modelMatrix, matrix);
		if ((mesh->collision(mouseRay, matrix) || selectedAxis == index) && !axisHovered)
		{
			if(selectedAxis == 0)
				shader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 0.25f, 1));
			else
				shader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1.5, 1.5, 0.65f, 1));
			if (ImGui::IsMouseDown(0) && selectedAxis == 0 && isMouseJustPressed)
			{
				axisDragged = true;
				axisClicked = true;
				selectedAxis = index;
			}
			axisHovered = true;
		}
		else
			shader->setUniform(SimpleShader::Uniforms::color, color);
		mesh->draw();
		if (!ImGui::IsMouseDown(0) && selectedAxis == index)
		{
			//mouseup
			selectedAxis = 0;
			axisDragged = false;
			axisReleased = true;
		}

	};

	if (mode == Mode::Scale)
		drawDirection(modelMatrix, Axis::X | Axis::Y | Axis::Z, glm::vec4(1.0f, 0.25f, 1.0f, 1));

	if(mode != Mode::TranslateY)
		drawDirection(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0, 0, -1)), Axis::X, glm::vec4(1, 0.25f, 0.25f, 1));
	drawDirection(modelMatrix, Axis::Y, glm::vec4(0.25f, 1, 0.25f, 1));
	if (mode != Mode::TranslateY)
		drawDirection(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0)), Axis::Z, glm::vec4(0.25f, 0.25f, 1, 1));

	isMouseJustPressed = !ImGui::IsMouseDown(0);
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


std::vector<glm::vec3> Gadget::ScaleMesh::buildVertices()
{
	int height = 20;
	int tip = 30;
	std::vector<glm::vec3> verts = math::AABB::box(glm::vec3(-2, 5, -2), glm::vec3(2, height, 2));
	auto b = math::AABB::box(glm::vec3(-5, height, -5), glm::vec3(5, height + 10, 5));
	verts.insert(verts.begin(), b.begin(), b.end());
	return verts;
}

std::vector<glm::vec3> Gadget::ScaleCubeMesh::buildVertices()
{
	return math::AABB::box(glm::vec3(-5, -5, -5), glm::vec3(5, 5, 5));
}

std::vector<glm::vec3> Gadget::RotateMesh::buildVertices()
{
	std::vector<glm::vec3> verts;
	/*verts.push_back(glm::vec3(0.836100, 0.835598, 10.379000)); verts.push_back(glm::vec3(0.769100, 3.931098, 8.954101)); verts.push_back(glm::vec3(-0.769100, 3.931098, 8.954101));
	verts.push_back(glm::vec3(-0.769100, 3.931098, 8.954101)); verts.push_back(glm::vec3(-0.836100, 0.835598, 10.379000)); verts.push_back(glm::vec3(0.836100, 0.835598, 10.379000));
	verts.push_back(glm::vec3(0.836100, 0.835598, 10.379000)); verts.push_back(glm::vec3(1.038300, 0.054898, 9.960600)); verts.push_back(glm::vec3(1.121500, 2.505899, 7.821900));
	verts.push_back(glm::vec3(1.121500, 2.505899, 7.821900)); verts.push_back(glm::vec3(0.769100, 3.931098, 8.954101)); verts.push_back(glm::vec3(0.836100, 0.835598, 10.379000));
	verts.push_back(glm::vec3(-1.038300, 0.054898, 9.960600)); verts.push_back(glm::vec3(-1.121500, 2.505899, 7.821900)); verts.push_back(glm::vec3(1.121500, 2.505899, 7.821900));
	verts.push_back(glm::vec3(1.121500, 2.505899, 7.821900)); verts.push_back(glm::vec3(1.038300, 0.054898, 9.960600)); verts.push_back(glm::vec3(-1.038300, 0.054898, 9.960600));
	verts.push_back(glm::vec3(-0.836100, 0.835598, 10.379000)); verts.push_back(glm::vec3(-0.769100, 3.931098, 8.954101)); verts.push_back(glm::vec3(-1.121500, 2.505899, 7.821900));
	verts.push_back(glm::vec3(-1.121500, 2.505899, 7.821900)); verts.push_back(glm::vec3(-1.038300, 0.054898, 9.960600)); verts.push_back(glm::vec3(-0.836100, 0.835598, 10.379000));
	verts.push_back(glm::vec3(0.769100, 3.931098, 8.954101)); verts.push_back(glm::vec3(0.769100, 6.375099, 6.836301)); verts.push_back(glm::vec3(-0.769100, 6.375099, 6.836301));
	verts.push_back(glm::vec3(-0.769100, 6.375099, 6.836301)); verts.push_back(glm::vec3(-0.769100, 3.931098, 8.954101)); verts.push_back(glm::vec3(0.769100, 3.931098, 8.954101));
	verts.push_back(glm::vec3(0.769100, 3.931098, 8.954101)); verts.push_back(glm::vec3(1.121500, 2.505899, 7.821900)); verts.push_back(glm::vec3(1.121500, 4.542399, 6.057301));
	verts.push_back(glm::vec3(1.121500, 4.542399, 6.057301)); verts.push_back(glm::vec3(0.769100, 6.375099, 6.836301)); verts.push_back(glm::vec3(0.769100, 3.931098, 8.954101));
	verts.push_back(glm::vec3(-1.121500, 2.505899, 7.821900)); verts.push_back(glm::vec3(-1.121500, 4.542399, 6.057301)); verts.push_back(glm::vec3(1.121500, 4.542399, 6.057301));
	verts.push_back(glm::vec3(1.121500, 4.542399, 6.057301)); verts.push_back(glm::vec3(1.121500, 2.505899, 7.821900)); verts.push_back(glm::vec3(-1.121500, 2.505899, 7.821900));
	verts.push_back(glm::vec3(-0.769100, 3.931098, 8.954101)); verts.push_back(glm::vec3(-0.769100, 6.375099, 6.836301)); verts.push_back(glm::vec3(-1.121500, 4.542399, 6.057301));
	verts.push_back(glm::vec3(-1.121500, 4.542399, 6.057301)); verts.push_back(glm::vec3(-1.121500, 2.505899, 7.821900)); verts.push_back(glm::vec3(-0.769100, 3.931098, 8.954101));
	verts.push_back(glm::vec3(0.769100, 6.375099, 6.836301)); verts.push_back(glm::vec3(0.769100, 8.123499, 4.115801)); verts.push_back(glm::vec3(-0.769100, 8.123499, 4.115801));
	verts.push_back(glm::vec3(-0.769100, 8.123499, 4.115801)); verts.push_back(glm::vec3(-0.769100, 6.375099, 6.836301)); verts.push_back(glm::vec3(0.769100, 6.375099, 6.836301));
	verts.push_back(glm::vec3(0.769100, 6.375099, 6.836301)); verts.push_back(glm::vec3(1.121500, 4.542399, 6.057301)); verts.push_back(glm::vec3(1.121500, 5.999199, 3.790501));
	verts.push_back(glm::vec3(1.121500, 5.999199, 3.790501)); verts.push_back(glm::vec3(0.769100, 8.123499, 4.115801)); verts.push_back(glm::vec3(0.769100, 6.375099, 6.836301));
	verts.push_back(glm::vec3(-1.121500, 4.542399, 6.057301)); verts.push_back(glm::vec3(-1.121500, 5.999199, 3.790501)); verts.push_back(glm::vec3(1.121500, 5.999199, 3.790501));
	verts.push_back(glm::vec3(1.121500, 5.999199, 3.790501)); verts.push_back(glm::vec3(1.121500, 4.542399, 6.057301)); verts.push_back(glm::vec3(-1.121500, 4.542399, 6.057301));
	verts.push_back(glm::vec3(-0.769100, 6.375099, 6.836301)); verts.push_back(glm::vec3(-0.769100, 8.123499, 4.115801)); verts.push_back(glm::vec3(-1.121500, 5.999199, 3.790501));
	verts.push_back(glm::vec3(-1.121500, 5.999199, 3.790501)); verts.push_back(glm::vec3(-1.121500, 4.542399, 6.057301)); verts.push_back(glm::vec3(-0.769100, 6.375099, 6.836301));
	verts.push_back(glm::vec3(0.769100, 8.123499, 4.115801)); verts.push_back(glm::vec3(0.769100, 9.034600, 1.012901)); verts.push_back(glm::vec3(-0.769100, 9.034600, 1.012901));
	verts.push_back(glm::vec3(-0.769100, 9.034600, 1.012901)); verts.push_back(glm::vec3(-0.769100, 8.123499, 4.115801)); verts.push_back(glm::vec3(0.769100, 8.123499, 4.115801));
	verts.push_back(glm::vec3(0.769100, 8.123499, 4.115801)); verts.push_back(glm::vec3(1.121500, 5.999199, 3.790501)); verts.push_back(glm::vec3(1.121500, 6.758300, 1.205101));
	verts.push_back(glm::vec3(1.121500, 6.758300, 1.205101)); verts.push_back(glm::vec3(0.769100, 9.034600, 1.012901)); verts.push_back(glm::vec3(0.769100, 8.123499, 4.115801));
	verts.push_back(glm::vec3(-1.121500, 5.999199, 3.790501)); verts.push_back(glm::vec3(-1.121500, 6.758300, 1.205101)); verts.push_back(glm::vec3(1.121500, 6.758300, 1.205101));
	verts.push_back(glm::vec3(1.121500, 6.758300, 1.205101)); verts.push_back(glm::vec3(1.121500, 5.999199, 3.790501)); verts.push_back(glm::vec3(-1.121500, 5.999199, 3.790501));
	verts.push_back(glm::vec3(-0.769100, 8.123499, 4.115801)); verts.push_back(glm::vec3(-0.769100, 9.034600, 1.012901)); verts.push_back(glm::vec3(-1.121500, 6.758300, 1.205101));
	verts.push_back(glm::vec3(-1.121500, 6.758300, 1.205101)); verts.push_back(glm::vec3(-1.121500, 5.999199, 3.790501)); verts.push_back(glm::vec3(-0.769100, 8.123499, 4.115801));
	verts.push_back(glm::vec3(0.769100, 9.034600, 1.012901)); verts.push_back(glm::vec3(0.769100, 9.034600, -2.220999)); verts.push_back(glm::vec3(-0.769100, 9.034600, -2.220999));
	verts.push_back(glm::vec3(-0.769100, 9.034600, -2.220999)); verts.push_back(glm::vec3(-0.769100, 9.034600, 1.012901)); verts.push_back(glm::vec3(0.769100, 9.034600, 1.012901));
	verts.push_back(glm::vec3(0.769100, 9.034600, 1.012901)); verts.push_back(glm::vec3(1.121500, 6.758300, 1.205101)); verts.push_back(glm::vec3(1.121500, 6.758300, -1.489499));
	verts.push_back(glm::vec3(1.121500, 6.758300, -1.489499)); verts.push_back(glm::vec3(0.769100, 9.034600, -2.220999)); verts.push_back(glm::vec3(0.769100, 9.034600, 1.012901));
	verts.push_back(glm::vec3(-1.121500, 6.758300, 1.205101)); verts.push_back(glm::vec3(-1.121500, 6.758300, -1.489499)); verts.push_back(glm::vec3(1.121500, 6.758300, -1.489499));
	verts.push_back(glm::vec3(1.121500, 6.758300, -1.489499)); verts.push_back(glm::vec3(1.121500, 6.758300, 1.205101)); verts.push_back(glm::vec3(-1.121500, 6.758300, 1.205101));
	verts.push_back(glm::vec3(-0.769100, 9.034600, 1.012901)); verts.push_back(glm::vec3(-0.769100, 9.034600, -2.220999)); verts.push_back(glm::vec3(-1.121500, 6.758300, -1.489499));
	verts.push_back(glm::vec3(-1.121500, 6.758300, -1.489499)); verts.push_back(glm::vec3(-1.121500, 6.758300, 1.205101)); verts.push_back(glm::vec3(-0.769100, 9.034600, 1.012901));
	verts.push_back(glm::vec3(0.769100, 9.034600, -2.220999)); verts.push_back(glm::vec3(0.769100, 8.123501, -5.323899)); verts.push_back(glm::vec3(-0.769100, 8.123501, -5.323899));
	verts.push_back(glm::vec3(-0.769100, 8.123501, -5.323899)); verts.push_back(glm::vec3(-0.769100, 9.034600, -2.220999)); verts.push_back(glm::vec3(0.769100, 9.034600, -2.220999));
	verts.push_back(glm::vec3(0.769100, 9.034600, -2.220999)); verts.push_back(glm::vec3(1.121500, 6.758300, -1.489499)); verts.push_back(glm::vec3(1.121500, 5.999200, -4.074899));
	verts.push_back(glm::vec3(1.121500, 5.999200, -4.074899)); verts.push_back(glm::vec3(0.769100, 8.123501, -5.323899)); verts.push_back(glm::vec3(0.769100, 9.034600, -2.220999));
	verts.push_back(glm::vec3(-1.121500, 6.758300, -1.489499)); verts.push_back(glm::vec3(-1.121500, 5.999200, -4.074899)); verts.push_back(glm::vec3(1.121500, 5.999200, -4.074899));
	verts.push_back(glm::vec3(1.121500, 5.999200, -4.074899)); verts.push_back(glm::vec3(1.121500, 6.758300, -1.489499)); verts.push_back(glm::vec3(-1.121500, 6.758300, -1.489499));
	verts.push_back(glm::vec3(-0.769100, 9.034600, -2.220999)); verts.push_back(glm::vec3(-0.769100, 8.123501, -5.323899)); verts.push_back(glm::vec3(-1.121500, 5.999200, -4.074899));
	verts.push_back(glm::vec3(-1.121500, 5.999200, -4.074899)); verts.push_back(glm::vec3(-1.121500, 6.758300, -1.489499)); verts.push_back(glm::vec3(-0.769100, 9.034600, -2.220999));
	verts.push_back(glm::vec3(0.769100, 8.123501, -5.323899)); verts.push_back(glm::vec3(0.769100, 6.375102, -8.044399)); verts.push_back(glm::vec3(-0.769100, 6.375102, -8.044399));
	verts.push_back(glm::vec3(-0.769100, 6.375102, -8.044399)); verts.push_back(glm::vec3(-0.769100, 8.123501, -5.323899)); verts.push_back(glm::vec3(0.769100, 8.123501, -5.323899));
	verts.push_back(glm::vec3(0.769100, 8.123501, -5.323899)); verts.push_back(glm::vec3(1.121500, 5.999200, -4.074899)); verts.push_back(glm::vec3(1.121500, 4.542401, -6.341699));
	verts.push_back(glm::vec3(1.121500, 4.542401, -6.341699)); verts.push_back(glm::vec3(0.769100, 6.375102, -8.044399)); verts.push_back(glm::vec3(0.769100, 8.123501, -5.323899));
	verts.push_back(glm::vec3(-1.121500, 5.999200, -4.074899)); verts.push_back(glm::vec3(-1.121500, 4.542401, -6.341699)); verts.push_back(glm::vec3(1.121500, 4.542401, -6.341699));
	verts.push_back(glm::vec3(1.121500, 4.542401, -6.341699)); verts.push_back(glm::vec3(1.121500, 5.999200, -4.074899)); verts.push_back(glm::vec3(-1.121500, 5.999200, -4.074899));
	verts.push_back(glm::vec3(-0.769100, 8.123501, -5.323899)); verts.push_back(glm::vec3(-0.769100, 6.375102, -8.044399)); verts.push_back(glm::vec3(-1.121500, 4.542401, -6.341699));
	verts.push_back(glm::vec3(-1.121500, 4.542401, -6.341699)); verts.push_back(glm::vec3(-1.121500, 5.999200, -4.074899)); verts.push_back(glm::vec3(-0.769100, 8.123501, -5.323899));
	verts.push_back(glm::vec3(0.769100, 6.375102, -8.044399)); verts.push_back(glm::vec3(0.769100, 3.931102, -10.162199)); verts.push_back(glm::vec3(-0.769100, 3.931102, -10.162199));
	verts.push_back(glm::vec3(-0.769100, 3.931102, -10.162199)); verts.push_back(glm::vec3(-0.769100, 6.375102, -8.044399)); verts.push_back(glm::vec3(0.769100, 6.375102, -8.044399));
	verts.push_back(glm::vec3(1.121500, 4.542401, -6.341699)); verts.push_back(glm::vec3(1.121500, 2.505901, -8.106300)); verts.push_back(glm::vec3(0.769100, 3.931102, -10.162199));
	verts.push_back(glm::vec3(0.769100, 3.931102, -10.162199)); verts.push_back(glm::vec3(0.769100, 6.375102, -8.044399)); verts.push_back(glm::vec3(1.121500, 4.542401, -6.341699));
	verts.push_back(glm::vec3(-1.121500, 4.542401, -6.341699)); verts.push_back(glm::vec3(-1.121500, 2.505901, -8.106300)); verts.push_back(glm::vec3(1.121500, 2.505901, -8.106300));
	verts.push_back(glm::vec3(1.121500, 2.505901, -8.106300)); verts.push_back(glm::vec3(1.121500, 4.542401, -6.341699)); verts.push_back(glm::vec3(-1.121500, 4.542401, -6.341699));
	verts.push_back(glm::vec3(-0.769100, 6.375102, -8.044399)); verts.push_back(glm::vec3(-0.769100, 3.931102, -10.162199)); verts.push_back(glm::vec3(-1.121500, 2.505901, -8.106300));
	verts.push_back(glm::vec3(-1.121500, 2.505901, -8.106300)); verts.push_back(glm::vec3(-1.121500, 4.542401, -6.341699)); verts.push_back(glm::vec3(-0.769100, 6.375102, -8.044399));
	verts.push_back(glm::vec3(0.769100, 3.931102, -10.162199)); verts.push_back(glm::vec3(0.769100, 0.989502, -11.505600)); verts.push_back(glm::vec3(-0.769100, 0.989502, -11.505600));
	verts.push_back(glm::vec3(-0.769100, 0.989502, -11.505600)); verts.push_back(glm::vec3(-0.769100, 3.931102, -10.162199)); verts.push_back(glm::vec3(0.769100, 3.931102, -10.162199));
	verts.push_back(glm::vec3(1.121500, 2.505901, -8.106300)); verts.push_back(glm::vec3(1.121500, 0.054902, -9.225700)); verts.push_back(glm::vec3(0.769100, 0.989502, -11.505600));
	verts.push_back(glm::vec3(0.769100, 0.989502, -11.505600)); verts.push_back(glm::vec3(0.769100, 3.931102, -10.162199)); verts.push_back(glm::vec3(1.121500, 2.505901, -8.106300));
	verts.push_back(glm::vec3(-1.121500, 2.505901, -8.106300)); verts.push_back(glm::vec3(-1.121500, 0.054902, -9.225700)); verts.push_back(glm::vec3(1.121500, 0.054902, -9.225700));
	verts.push_back(glm::vec3(1.121500, 0.054902, -9.225700)); verts.push_back(glm::vec3(1.121500, 2.505901, -8.106300)); verts.push_back(glm::vec3(-1.121500, 2.505901, -8.106300));
	verts.push_back(glm::vec3(-1.121500, 2.505901, -8.106300)); verts.push_back(glm::vec3(-0.769100, 3.931102, -10.162199)); verts.push_back(glm::vec3(-0.769100, 0.989502, -11.505600));
	verts.push_back(glm::vec3(-0.769100, 0.989502, -11.505600)); verts.push_back(glm::vec3(-1.121500, 0.054902, -9.225700)); verts.push_back(glm::vec3(-1.121500, 2.505901, -8.106300));
	verts.push_back(glm::vec3(0.769100, 0.989502, -11.505600)); verts.push_back(glm::vec3(0.769100, -2.211498, -11.965800)); verts.push_back(glm::vec3(-0.769100, -2.211498, -11.965800));
	verts.push_back(glm::vec3(-0.769100, -2.211498, -11.965800)); verts.push_back(glm::vec3(-0.769100, 0.989502, -11.505600)); verts.push_back(glm::vec3(0.769100, 0.989502, -11.505600));
	verts.push_back(glm::vec3(1.121500, 0.054902, -9.225700)); verts.push_back(glm::vec3(1.121500, -2.612298, -9.609100)); verts.push_back(glm::vec3(0.769100, -2.211498, -11.965800));
	verts.push_back(glm::vec3(0.769100, -2.211498, -11.965800)); verts.push_back(glm::vec3(0.769100, 0.989502, -11.505600)); verts.push_back(glm::vec3(1.121500, 0.054902, -9.225700));
	verts.push_back(glm::vec3(-1.121500, 0.054902, -9.225700)); verts.push_back(glm::vec3(-1.121500, -2.612298, -9.609100)); verts.push_back(glm::vec3(1.121500, -2.612298, -9.609100));
	verts.push_back(glm::vec3(1.121500, -2.612298, -9.609100)); verts.push_back(glm::vec3(1.121500, 0.054902, -9.225700)); verts.push_back(glm::vec3(-1.121500, 0.054902, -9.225700));
	verts.push_back(glm::vec3(-1.121500, 0.054902, -9.225700)); verts.push_back(glm::vec3(-0.769100, 0.989502, -11.505600)); verts.push_back(glm::vec3(-0.769100, -2.211498, -11.965800));
	verts.push_back(glm::vec3(-0.769100, -2.211498, -11.965800)); verts.push_back(glm::vec3(-1.121500, -2.612298, -9.609100)); verts.push_back(glm::vec3(-1.121500, 0.054902, -9.225700));
	verts.push_back(glm::vec3(0.769100, -2.211498, -11.965800)); verts.push_back(glm::vec3(0.769100, -5.412498, -11.505601)); verts.push_back(glm::vec3(-0.769100, -5.412498, -11.505601));
	verts.push_back(glm::vec3(-0.769100, -5.412498, -11.505601)); verts.push_back(glm::vec3(-0.769100, -2.211498, -11.965800)); verts.push_back(glm::vec3(0.769100, -2.211498, -11.965800));
	verts.push_back(glm::vec3(1.121500, -2.612298, -9.609100)); verts.push_back(glm::vec3(1.121500, -5.279398, -9.225701)); verts.push_back(glm::vec3(0.769100, -5.412498, -11.505601));
	verts.push_back(glm::vec3(0.769100, -5.412498, -11.505601)); verts.push_back(glm::vec3(0.769100, -2.211498, -11.965800)); verts.push_back(glm::vec3(1.121500, -2.612298, -9.609100));
	verts.push_back(glm::vec3(-1.121500, -2.612298, -9.609100)); verts.push_back(glm::vec3(-1.121500, -5.279398, -9.225701)); verts.push_back(glm::vec3(1.121500, -5.279398, -9.225701));
	verts.push_back(glm::vec3(1.121500, -5.279398, -9.225701)); verts.push_back(glm::vec3(1.121500, -2.612298, -9.609100)); verts.push_back(glm::vec3(-1.121500, -2.612298, -9.609100));
	verts.push_back(glm::vec3(-1.121500, -2.612298, -9.609100)); verts.push_back(glm::vec3(-0.769100, -2.211498, -11.965800)); verts.push_back(glm::vec3(-0.769100, -5.412498, -11.505601));
	verts.push_back(glm::vec3(-0.769100, -5.412498, -11.505601)); verts.push_back(glm::vec3(-1.121500, -5.279398, -9.225701)); verts.push_back(glm::vec3(-1.121500, -2.612298, -9.609100));
	verts.push_back(glm::vec3(0.769100, -5.412498, -11.505601)); verts.push_back(glm::vec3(0.769100, -8.354198, -10.162201)); verts.push_back(glm::vec3(-0.769100, -8.354198, -10.162201));
	verts.push_back(glm::vec3(-0.769100, -8.354198, -10.162201)); verts.push_back(glm::vec3(-0.769100, -5.412498, -11.505601)); verts.push_back(glm::vec3(0.769100, -5.412498, -11.505601));
	verts.push_back(glm::vec3(1.121500, -5.279398, -9.225701)); verts.push_back(glm::vec3(1.121500, -7.730499, -8.106301)); verts.push_back(glm::vec3(0.769100, -8.354198, -10.162201));
	verts.push_back(glm::vec3(0.769100, -8.354198, -10.162201)); verts.push_back(glm::vec3(0.769100, -5.412498, -11.505601)); verts.push_back(glm::vec3(1.121500, -5.279398, -9.225701));
	verts.push_back(glm::vec3(-1.121500, -5.279398, -9.225701)); verts.push_back(glm::vec3(-1.121500, -7.730499, -8.106301)); verts.push_back(glm::vec3(1.121500, -7.730499, -8.106301));
	verts.push_back(glm::vec3(1.121500, -7.730499, -8.106301)); verts.push_back(glm::vec3(1.121500, -5.279398, -9.225701)); verts.push_back(glm::vec3(-1.121500, -5.279398, -9.225701));
	verts.push_back(glm::vec3(-1.121500, -5.279398, -9.225701)); verts.push_back(glm::vec3(-0.769100, -5.412498, -11.505601)); verts.push_back(glm::vec3(-0.769100, -8.354198, -10.162201));
	verts.push_back(glm::vec3(-0.769100, -8.354198, -10.162201)); verts.push_back(glm::vec3(-1.121500, -7.730499, -8.106301)); verts.push_back(glm::vec3(-1.121500, -5.279398, -9.225701));
	verts.push_back(glm::vec3(0.769100, -8.354198, -10.162201)); verts.push_back(glm::vec3(0.769100, -10.798199, -8.044402)); verts.push_back(glm::vec3(-0.769100, -10.798199, -8.044402));
	verts.push_back(glm::vec3(-0.769100, -10.798199, -8.044402)); verts.push_back(glm::vec3(-0.769100, -8.354198, -10.162201)); verts.push_back(glm::vec3(0.769100, -8.354198, -10.162201));
	verts.push_back(glm::vec3(1.121500, -7.730499, -8.106301)); verts.push_back(glm::vec3(1.121500, -9.766899, -6.341702)); verts.push_back(glm::vec3(0.769100, -10.798199, -8.044402));
	verts.push_back(glm::vec3(0.769100, -10.798199, -8.044402)); verts.push_back(glm::vec3(0.769100, -8.354198, -10.162201)); verts.push_back(glm::vec3(1.121500, -7.730499, -8.106301));
	verts.push_back(glm::vec3(-1.121500, -7.730499, -8.106301)); verts.push_back(glm::vec3(-1.121500, -9.766899, -6.341702)); verts.push_back(glm::vec3(1.121500, -9.766899, -6.341702));
	verts.push_back(glm::vec3(1.121500, -9.766899, -6.341702)); verts.push_back(glm::vec3(1.121500, -7.730499, -8.106301)); verts.push_back(glm::vec3(-1.121500, -7.730499, -8.106301));
	verts.push_back(glm::vec3(-1.121500, -7.730499, -8.106301)); verts.push_back(glm::vec3(-0.769100, -8.354198, -10.162201)); verts.push_back(glm::vec3(-0.769100, -10.798199, -8.044402));
	verts.push_back(glm::vec3(-0.769100, -10.798199, -8.044402)); verts.push_back(glm::vec3(-1.121500, -9.766899, -6.341702)); verts.push_back(glm::vec3(-1.121500, -7.730499, -8.106301));
	verts.push_back(glm::vec3(0.769100, -10.798199, -8.044402)); verts.push_back(glm::vec3(0.769100, -12.546599, -5.323902)); verts.push_back(glm::vec3(-0.769100, -12.546599, -5.323902));
	verts.push_back(glm::vec3(-0.769100, -12.546599, -5.323902)); verts.push_back(glm::vec3(-0.769100, -10.798199, -8.044402)); verts.push_back(glm::vec3(0.769100, -10.798199, -8.044402));
	verts.push_back(glm::vec3(1.121500, -9.766899, -6.341702)); verts.push_back(glm::vec3(1.121500, -11.223699, -4.074902)); verts.push_back(glm::vec3(0.769100, -12.546599, -5.323902));
	verts.push_back(glm::vec3(0.769100, -12.546599, -5.323902)); verts.push_back(glm::vec3(0.769100, -10.798199, -8.044402)); verts.push_back(glm::vec3(1.121500, -9.766899, -6.341702));
	verts.push_back(glm::vec3(-1.121500, -9.766899, -6.341702)); verts.push_back(glm::vec3(-1.121500, -11.223699, -4.074902)); verts.push_back(glm::vec3(1.121500, -11.223699, -4.074902));
	verts.push_back(glm::vec3(1.121500, -11.223699, -4.074902)); verts.push_back(glm::vec3(1.121500, -9.766899, -6.341702)); verts.push_back(glm::vec3(-1.121500, -9.766899, -6.341702));
	verts.push_back(glm::vec3(-1.121500, -9.766899, -6.341702)); verts.push_back(glm::vec3(-0.769100, -10.798199, -8.044402)); verts.push_back(glm::vec3(-0.769100, -12.546599, -5.323902));
	verts.push_back(glm::vec3(-0.769100, -12.546599, -5.323902)); verts.push_back(glm::vec3(-1.121500, -11.223699, -4.074902)); verts.push_back(glm::vec3(-1.121500, -9.766899, -6.341702));
	verts.push_back(glm::vec3(0.769100, -12.546599, -5.323902)); verts.push_back(glm::vec3(0.659300, -13.255600, -1.459902)); verts.push_back(glm::vec3(-0.659300, -13.255600, -1.459902));
	verts.push_back(glm::vec3(-0.659300, -13.255600, -1.459902)); verts.push_back(glm::vec3(-0.769100, -12.546599, -5.323902)); verts.push_back(glm::vec3(0.769100, -12.546599, -5.323902));
	verts.push_back(glm::vec3(1.121500, -11.223699, -4.074902)); verts.push_back(glm::vec3(0.818800, -12.103900, -1.459902)); verts.push_back(glm::vec3(0.659300, -13.255600, -1.459902));
	verts.push_back(glm::vec3(0.659300, -13.255600, -1.459902)); verts.push_back(glm::vec3(0.769100, -12.546599, -5.323902)); verts.push_back(glm::vec3(1.121500, -11.223699, -4.074902));
	verts.push_back(glm::vec3(-1.121500, -11.223699, -4.074902)); verts.push_back(glm::vec3(-0.818800, -12.103900, -1.459902)); verts.push_back(glm::vec3(0.818800, -12.103900, -1.459902));
	verts.push_back(glm::vec3(0.818800, -12.103900, -1.459902)); verts.push_back(glm::vec3(1.121500, -11.223699, -4.074902)); verts.push_back(glm::vec3(-1.121500, -11.223699, -4.074902));
	verts.push_back(glm::vec3(-1.121500, -11.223699, -4.074902)); verts.push_back(glm::vec3(-0.769100, -12.546599, -5.323902)); verts.push_back(glm::vec3(-0.659300, -13.255600, -1.459902));
	verts.push_back(glm::vec3(-0.659300, -13.255600, -1.459902)); verts.push_back(glm::vec3(-0.818800, -12.103900, -1.459902)); verts.push_back(glm::vec3(-1.121500, -11.223699, -4.074902));
	verts.push_back(glm::vec3(0.659300, -13.255600, -1.459902)); verts.push_back(glm::vec3(2.223700, -15.050400, -1.459903)); verts.push_back(glm::vec3(-2.223700, -15.050400, -1.459903));
	verts.push_back(glm::vec3(-2.223700, -15.050400, -1.459903)); verts.push_back(glm::vec3(-0.659300, -13.255600, -1.459902)); verts.push_back(glm::vec3(0.659300, -13.255600, -1.459902));
	verts.push_back(glm::vec3(0.818800, -12.103900, -1.459902)); verts.push_back(glm::vec3(2.223700, -9.904400, -1.459902)); verts.push_back(glm::vec3(2.223700, -15.050400, -1.459903));
	verts.push_back(glm::vec3(2.223700, -15.050400, -1.459903)); verts.push_back(glm::vec3(0.659300, -13.255600, -1.459902)); verts.push_back(glm::vec3(0.818800, -12.103900, -1.459902));
	verts.push_back(glm::vec3(-0.818800, -12.103900, -1.459902)); verts.push_back(glm::vec3(-2.223700, -9.904400, -1.459902)); verts.push_back(glm::vec3(2.223700, -9.904400, -1.459902));
	verts.push_back(glm::vec3(2.223700, -9.904400, -1.459902)); verts.push_back(glm::vec3(0.818800, -12.103900, -1.459902)); verts.push_back(glm::vec3(-0.818800, -12.103900, -1.459902));
	verts.push_back(glm::vec3(-0.818800, -12.103900, -1.459902)); verts.push_back(glm::vec3(-0.659300, -13.255600, -1.459902)); verts.push_back(glm::vec3(-2.223700, -15.050400, -1.459903));
	verts.push_back(glm::vec3(-2.223700, -15.050400, -1.459903)); verts.push_back(glm::vec3(-2.223700, -9.904400, -1.459902)); verts.push_back(glm::vec3(-0.818800, -12.103900, -1.459902));
	verts.push_back(glm::vec3(-2.223700, -15.050400, -1.459903)); verts.push_back(glm::vec3(2.223700, -15.050400, -1.459903)); verts.push_back(glm::vec3(0.000000, -12.477401, 7.080598));
	verts.push_back(glm::vec3(2.223700, -15.050400, -1.459903)); verts.push_back(glm::vec3(2.223700, -9.904400, -1.459902)); verts.push_back(glm::vec3(0.000000, -12.477401, 7.080598));
	verts.push_back(glm::vec3(2.223700, -9.904400, -1.459902)); verts.push_back(glm::vec3(-2.223700, -9.904400, -1.459902)); verts.push_back(glm::vec3(0.000000, -12.477401, 7.080598));
	verts.push_back(glm::vec3(-2.223700, -9.904400, -1.459902)); verts.push_back(glm::vec3(-2.223700, -15.050400, -1.459903)); verts.push_back(glm::vec3(0.000000, -12.477401, 7.080598));
	verts.push_back(glm::vec3(0.836100, 0.835598, 10.379000)); verts.push_back(glm::vec3(-0.836100, 0.835598, 10.379000)); verts.push_back(glm::vec3(-1.038300, 0.054898, 9.960600));
	verts.push_back(glm::vec3(-1.038300, 0.054898, 9.960600)); verts.push_back(glm::vec3(1.038300, 0.054898, 9.960600)); verts.push_back(glm::vec3(0.836100, 0.835598, 10.379000));

	for (std::size_t i = 0; i < verts.size(); i++)
	{
		verts[i] = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1)) * glm::vec4(verts[i], 1.0f));
	}*/

	float inc = (2 * glm::pi<float>()) / 36.0f;
	float w = 1;
	float size = 25;
	float siz2 = size - 2*w;

	for (float f = 0; f < 2 * glm::pi<float>(); f += inc)
	{
		glm::vec3 v1(size * glm::cos(f), -w, size * glm::sin(f));
		glm::vec3 v2(size * glm::cos(f+inc), -w, size * glm::sin(f+inc));
		glm::vec3 v3(size * glm::cos(f+inc), w, size * glm::sin(f+inc));
		glm::vec3 v4(size * glm::cos(f), w, size * glm::sin(f));

		glm::vec3 v5(siz2 * glm::cos(f), -w, siz2 * glm::sin(f));
		glm::vec3 v6(siz2 * glm::cos(f + inc), -w, siz2 * glm::sin(f + inc));
		glm::vec3 v7(siz2 * glm::cos(f + inc), w, siz2 * glm::sin(f + inc));
		glm::vec3 v8(siz2 * glm::cos(f), w, siz2 * glm::sin(f));
		//outside
		verts.push_back(v3);
		verts.push_back(v2);
		verts.push_back(v1);
		verts.push_back(v1);
		verts.push_back(v4);
		verts.push_back(v3);
		//inside
		verts.push_back(v5);
		verts.push_back(v6);
		verts.push_back(v7);
		verts.push_back(v7);
		verts.push_back(v8);
		verts.push_back(v5);
		//sides
		verts.push_back(v1);
		verts.push_back(v2);
		verts.push_back(v6);
		verts.push_back(v1);
		verts.push_back(v6);
		verts.push_back(v5);

		verts.push_back(v3);
		verts.push_back(v4);
		verts.push_back(v8);
		verts.push_back(v3);
		verts.push_back(v8);
		verts.push_back(v7);

	}

	return verts;
}
