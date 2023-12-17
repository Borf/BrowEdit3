#include "Gadget.h"
#include <browedit/math/AABB.h>
#include <browedit/shaders/SimpleShader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

void Gadget::init()
{
	shader = new SimpleShader();
	shader->use();
	shader->setUniform(SimpleShader::Uniforms::s_texture, 0);

	arrowMesh.init();
	rotateMesh.init(false);
	scaleMesh.init();
	scaleCubeMesh.init();
}

void Gadget::setMatrices(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix, const glm::vec3& lightDirection)
{
	Gadget::projectionMatrix = projectionMatrix;
	Gadget::viewMatrix = viewMatrix;
	Gadget::lightDirection = lightDirection;
}

void Gadget::draw(const math::Ray& mouseRay, glm::mat4 modelMatrix, float angleX)
{
	axisClicked = false;
	axisReleased = false;
	axisHovered = false;
	hoveredAxis = 0;

	modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));

	shader->use();
	shader->setUniform(SimpleShader::Uniforms::projectionMatrix, projectionMatrix);
	shader->setUniform(SimpleShader::Uniforms::viewMatrix, viewMatrix);
	shader->setUniform(SimpleShader::Uniforms::modelMatrix, modelMatrix);
	shader->setUniform(SimpleShader::Uniforms::lightDirection, lightDirection);
	shader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
	float t = 9999;

	auto detectHover = [&](const glm::mat4& matrix, int index, const glm::vec4& color)
	{
		Mesh* mesh = nullptr;
		if (mode == Mode::Translate)	mesh = &arrowMesh;
		if (mode == Mode::TranslateY)	mesh = &arrowMesh;
		if (mode == Mode::Rotate)		mesh = &rotateMesh;
		if (mode == Mode::Scale)		mesh = &scaleMesh;
		if (mode == Mode::Scale && index != 0 && (index & (index - 1)) != 0)		mesh = &scaleCubeMesh;

		float tt;
		if ((mesh->collision(mouseRay, matrix, tt) || selectedAxis == index) && tt < t && !disabled)
		{
			t = tt;
			hoveredAxis = index;
		}
	};

	auto drawDirection = [&](const glm::mat4& matrix, int index, const glm::vec4& color)
	{
		Mesh* mesh = nullptr;
		if (mode == Mode::Translate)	mesh = &arrowMesh;
		if (mode == Mode::TranslateY)	mesh = &arrowMesh;
		if (mode == Mode::Rotate)		mesh = &rotateMesh;
		if (mode == Mode::Scale)		mesh = &scaleMesh;
		if (mode == Mode::Scale && index != 0 && (index & (index - 1)) != 0)		mesh = &scaleCubeMesh;

		shader->setUniform(SimpleShader::Uniforms::modelMatrix, matrix);

		if (!disabled && ((hoveredAxis == 0 && (selectedAxis == index || mesh->collision(mouseRay, matrix, t))) || hoveredAxis == index))
		{
			if(selectedAxis == 0)
				shader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 0.25f, 1));
			else
				shader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1.5, 1.5, 0.65f, 1));
			if (ImGui::IsMouseDown(0) && selectedAxis == 0 && ImGui::IsMouseClicked(0))
			{
				axisDragged = true;
				axisClicked = true;
				selectedAxis = index;
			}
			axisHovered = true;
			hoveredAxis = index;
		} else if(disabled)
			shader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0.5f, 0.5f, 0.5f, 0.5f * opacity));
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

	if (mode == Mode::Rotate) {
		int drawAxis = Axis::X | Axis::Y | Axis::Z;

		if (axisDragged || axisClicked)
			drawAxis = selectedAxis;

		if ((drawAxis & Axis::X) != 0)
			detectHover(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0, 0, -1)), Axis::X, glm::vec4(0));
		if ((drawAxis & Axis::Y) != 0)
			detectHover(glm::rotate(modelMatrix, glm::radians(angleX), glm::vec3(1, 0, 0)), Axis::Y, glm::vec4(0));
		if ((drawAxis & Axis::Z) != 0)
			detectHover(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0)), Axis::Z, glm::vec4(0));

		glEnable(GL_CULL_FACE);
		if ((drawAxis & Axis::X) != 0)
			drawDirection(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0, 0, -1)), Axis::X, glm::vec4(1, 0.25f, 0.25f, opacity));
		if ((drawAxis & Axis::Y) != 0)
			drawDirection(glm::rotate(modelMatrix, glm::radians(angleX), glm::vec3(1, 0, 0)), Axis::Y, glm::vec4(0.25f, 1, 0.25f, opacity));
		if ((drawAxis & Axis::Z) != 0)
			drawDirection(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0)), Axis::Z, glm::vec4(0.25f, 0.25f, 1, opacity));
		glDisable(GL_CULL_FACE);
		if (!axisHovered)
			hoveredAxis = 0;
	}
	else if (mode == Mode::TranslateY) {
		drawDirection(modelMatrix, Axis::Y, glm::vec4(0.25f, 1, 0.25f, opacity));
	}
	else {
		if (mode == Mode::Scale)
			drawDirection(modelMatrix, Axis::X | Axis::Y | Axis::Z, glm::vec4(1.0f, 0.25f, 1.0f, opacity));

		drawDirection(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(0, 0, -1)), Axis::X, glm::vec4(1, 0.25f, 0.25f, opacity));
		drawDirection(modelMatrix, Axis::Y, glm::vec4(0.25f, 1, 0.25f, opacity));
		drawDirection(glm::rotate(modelMatrix, glm::radians(90.0f), glm::vec3(1, 0, 0)), Axis::Z, glm::vec4(0.25f, 0.25f, 1, opacity));
	}

	isMouseJustPressed = !ImGui::IsMouseDown(0);
}



void Gadget::setThickness(float newThickness)
{
	Gadget::thickness = newThickness;
	arrowMesh.init();
	rotateMesh.init(false);
	scaleMesh.init();
	scaleCubeMesh.init();
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



void Gadget::ArrowMesh::buildVertices(std::vector<VertexP3T2N3>& verts)
{
	int height = 20;
	int tip = 30;

	for (const auto& v : math::AABB::box(glm::vec3(-Gadget::thickness, 5, -Gadget::thickness), glm::vec3(Gadget::thickness, height, Gadget::thickness)))
		verts.push_back(v);
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
}


void Gadget::ScaleMesh::buildVertices(std::vector<VertexP3T2N3>& verts)
{
	int height = 20;
	int tip = 30;
	for (const auto& v : math::AABB::box(glm::vec3(-Gadget::thickness, 5, -Gadget::thickness), glm::vec3(Gadget::thickness, height, Gadget::thickness)))
		verts.push_back(v);
	auto b = math::AABB::box(glm::vec3(-5, height, -5), glm::vec3(5, height + 10, 5));
	verts.insert(verts.begin(), b.begin(), b.end());
}

void Gadget::ScaleCubeMesh::buildVertices(std::vector<VertexP3T2N3>& verts)
{
	for (const auto& v : math::AABB::box(glm::vec3(-5, -5, -5), glm::vec3(5, 5, 5)))
		verts.push_back(v);
}

void Gadget::RotateMesh::buildVertices(std::vector<VertexP3T2N3>& verts)
{
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
	float w = Gadget::thickness;
	float size = 25;
	float siz2 = size - 2*w;

	std::vector<glm::vec3> normals;

	for (float f = 0; f < 2 * glm::pi<float>(); f += inc)
	{
		glm::vec3 v1(size * glm::cos(f), -w, size * glm::sin(f));
		glm::vec3 v2(size * glm::cos(f + inc), -w, size * glm::sin(f + inc));
		glm::vec3 v3(size * glm::cos(f + inc), w, size * glm::sin(f + inc));
		glm::vec3 v4(size * glm::cos(f), w, size * glm::sin(f));

		glm::vec3 v5(siz2 * glm::cos(f), -w, siz2 * glm::sin(f));
		glm::vec3 v6(siz2 * glm::cos(f + inc), -w, siz2 * glm::sin(f + inc));
		glm::vec3 v7(siz2 * glm::cos(f + inc), w, siz2 * glm::sin(f + inc));
		glm::vec3 v8(siz2 * glm::cos(f), w, siz2 * glm::sin(f));
		
		normals.push_back(-glm::cross(glm::make_vec3(v2) - glm::make_vec3(v1), glm::make_vec3(v3) - glm::make_vec3(v1)));

		//outside
		verts.push_back(v1);
		verts.push_back(v2);
		verts.push_back(v3);
		verts.push_back(v3);
		verts.push_back(v4);
		verts.push_back(v1);

		//inside
		verts.push_back(v7);
		verts.push_back(v6);
		verts.push_back(v5);
		verts.push_back(v5);
		verts.push_back(v8);
		verts.push_back(v7);
		
		//sides
		verts.push_back(v6);
		verts.push_back(v2);
		verts.push_back(v1);
		verts.push_back(v5);
		verts.push_back(v6);
		verts.push_back(v1);
		
		verts.push_back(v8);
		verts.push_back(v4);
		verts.push_back(v3);
		verts.push_back(v7);
		verts.push_back(v8);
		verts.push_back(v3);
	}

	for (size_t i = 0, j = 0; i < verts.size(); i += 24, j++)
	{
		glm::vec3 normal_t = glm::normalize(normals[j] + (j == 0 ? normals[normals.size() - 1] : normals[j - 1]));
		glm::vec3 normal_b = glm::normalize(normals[j] + (j == normals.size() - 1 ? normals[0] : normals[j + 1]));

		// normals outside
		verts[i + 0].data[3 + 2 + 0] = normal_t.x;
		verts[i + 0].data[3 + 2 + 1] = normal_t.y;
		verts[i + 0].data[3 + 2 + 2] = normal_t.z;
		
		verts[i + 1].data[3 + 2 + 0] = normal_b.x;
		verts[i + 1].data[3 + 2 + 1] = normal_b.y;
		verts[i + 1].data[3 + 2 + 2] = normal_b.z;
		
		verts[i + 2].data[3 + 2 + 0] = normal_b.x;
		verts[i + 2].data[3 + 2 + 1] = normal_b.y;
		verts[i + 2].data[3 + 2 + 2] = normal_b.z;
		
		verts[i + 3].data[3 + 2 + 0] = normal_b.x;
		verts[i + 3].data[3 + 2 + 1] = normal_b.y;
		verts[i + 3].data[3 + 2 + 2] = normal_b.z;
		
		verts[i + 4].data[3 + 2 + 0] = normal_t.x;
		verts[i + 4].data[3 + 2 + 1] = normal_t.y;
		verts[i + 4].data[3 + 2 + 2] = normal_t.z;
		
		verts[i + 5].data[3 + 2 + 0] = normal_t.x;
		verts[i + 5].data[3 + 2 + 1] = normal_t.y;
		verts[i + 5].data[3 + 2 + 2] = normal_t.z;

		// normals inside
		verts[i + 6].data[3 + 2 + 0] = -normal_b.x;
		verts[i + 6].data[3 + 2 + 1] = -normal_b.y;
		verts[i + 6].data[3 + 2 + 2] = -normal_b.z;
		
		verts[i + 7].data[3 + 2 + 0] = -normal_b.x;
		verts[i + 7].data[3 + 2 + 1] = -normal_b.y;
		verts[i + 7].data[3 + 2 + 2] = -normal_b.z;
		
		verts[i + 8].data[3 + 2 + 0] = -normal_t.x;
		verts[i + 8].data[3 + 2 + 1] = -normal_t.y;
		verts[i + 8].data[3 + 2 + 2] = -normal_t.z;
		
		verts[i + 9].data[3 + 2 + 0] = -normal_t.x;
		verts[i + 9].data[3 + 2 + 1] = -normal_t.y;
		verts[i + 9].data[3 + 2 + 2] = -normal_t.z;
		
		verts[i + 10].data[3 + 2 + 0] = -normal_t.x;
		verts[i + 10].data[3 + 2 + 1] = -normal_t.y;
		verts[i + 10].data[3 + 2 + 2] = -normal_t.z;
		
		verts[i + 11].data[3 + 2 + 0] = -normal_b.x;
		verts[i + 11].data[3 + 2 + 1] = -normal_b.y;
		verts[i + 11].data[3 + 2 + 2] = -normal_b.z;

		// normals side
		for (size_t k = i + 12; k < i + 24; k += 3) {
			glm::vec3 normal = -glm::normalize(glm::cross(glm::make_vec3(verts[k + 1].data) - glm::make_vec3(verts[k].data), glm::make_vec3(verts[k + 2].data) - glm::make_vec3(verts[k].data)));
		
			for (size_t ii = k; ii < k + 3; ii++)
			{
				verts[ii].data[3 + 2 + 0] = normal.x;
				verts[ii].data[3 + 2 + 1] = normal.y;
				verts[ii].data[3 + 2 + 2] = normal.z;
			}
		}
	}
}
