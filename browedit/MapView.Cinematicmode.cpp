#include <Windows.h>
#include "MapView.h"
#include <browedit/BrowEdit.h>
#include <browedit/shaders/SimpleShader.h>
#include <browedit/gl/FBO.h>
#include <browedit/Map.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/Node.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/math/HermiteCurve.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

extern float timeSelected;


void MapView::postRenderCinematicMode(BrowEdit* browEdit)
{
	if (cinematicPlay)
		return;
	auto rsw = map->rootNode->getComponent<Rsw>();
	if (rsw->cinematicTracks.size() == 0)
		return;
	auto gnd = map->rootNode->getComponent<Gnd>();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();

	glUseProgram(0);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(nodeRenderContext.projectionMatrix));
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(nodeRenderContext.viewMatrix));
	glColor3f(1, 0, 0);
	fbo->bind();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_2D);

	simpleShader->use();
	simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
	simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
	glEnable(GL_BLEND);


	auto mouse3D = gnd->rayCast(mouseRay, viewEmptyTiles);
	glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));

	ImGui::Begin("Statusbar");
	ImGui::SetNextItemWidth(100.0f);
	ImGui::Text("Cursor: %d,%d", tileHovered.x, tileHovered.y);
	ImGui::SameLine();
	ImGui::End();

	std::vector<VertexP3T2N3> verts;
	for (float t = 0; t < 20; t += 0.05f)
	{
		auto beforePos = (Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>*)rsw->cinematicTracks[0].getBeforeFrame(t);
		auto afterPos = (Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>*)rsw->cinematicTracks[0].getAfterFrame(t);
		//assert(beforePos->time <= time);

		float frameTime = afterPos->time - beforePos->time;
		float mixFactor = (t - beforePos->time) / frameTime;

		//TODO: cache this!!!
		float segmentLength = math::HermiteCurve::getLength(beforePos->data.first, beforePos->data.second, afterPos->data.first, afterPos->data.second);
		glm::vec3 pos = math::HermiteCurve::getPointAtDistance(beforePos->data.first, beforePos->data.second, afterPos->data.first, afterPos->data.second, mixFactor * segmentLength);
		verts.push_back(VertexP3T2N3(pos, glm::vec2(0), glm::vec3(1,1,1)));
	}

	{
		auto beforePos = (Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>*)rsw->cinematicTracks[0].getBeforeFrame(timeSelected);
		auto afterPos = (Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>*)rsw->cinematicTracks[0].getAfterFrame(timeSelected);
		float frameTime = afterPos->time - beforePos->time;
		float mixFactor = (timeSelected - beforePos->time) / frameTime;
		float segmentLength = math::HermiteCurve::getLength(beforePos->data.first, beforePos->data.second, afterPos->data.first, afterPos->data.second);
		glm::vec3 pos = math::HermiteCurve::getPointAtDistance(beforePos->data.first, beforePos->data.second, afterPos->data.first, afterPos->data.second, mixFactor * segmentLength);

		for (float x = -1; x <= 1; x += 0.1f)
			for (float y = -1; y <= 1; y += 0.1f)
				for (float z = -1; z <= 1; z += 0.1f)
					verts.push_back(VertexP3T2N3(pos+glm::vec3(x,y,z), glm::vec2(0), glm::vec3(1, 1, 1)));

	}

	if (verts.size() > 0)
	{
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4); //TODO: vao
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);

		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0, 1, 0, 1));
		simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
		simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
		glDrawArrays(GL_POINTS, 0, (int)verts.size());
	}

	Gadget::setMatrices(nodeRenderContext.projectionMatrix, nodeRenderContext.viewMatrix);
	bool canSelectObject = true;
	float gridSize = gridSizeTranslate;
	float gridOffset = gridOffsetTranslate;
	static glm::vec3 originalPos;
	static std::map<Rsw::KeyFrame*, Gadget> gadgetMap;
	for (auto f : rsw->cinematicTracks[0].frames)
	{
		if (gadgetMap.find(f) == gadgetMap.end())
			gadgetMap[f] = Gadget();
		auto& g = gadgetMap[f];
		auto frame = dynamic_cast<Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>*>(f);
		glm::vec3 &pos = frame->data.first;
		glm::vec3& direction = frame->data.second;

		//glm::mat4 mat = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, -1));
		glm::mat4 mat = glm::translate(glm::mat4(1.0f), pos);
		g.mode = Gadget::Mode::Translate;
		g.draw(mouseRay, mat);
		if (hovered)
		{
			if (g.axisClicked && canSelectObject)
			{
				mouseDragPlane.normal = glm::normalize(glm::vec3(nodeRenderContext.viewMatrix * glm::vec4(0, 0, 1, 1)) - glm::vec3(nodeRenderContext.viewMatrix * glm::vec4(0, 0, 0, 1))) * glm::vec3(1, 1, -1);
				if (g.selectedAxis == Gadget::Axis::X)
					mouseDragPlane.normal.x = 0;
				if (g.selectedAxis == Gadget::Axis::Y)
					mouseDragPlane.normal.y = 0;
				if (g.selectedAxis == Gadget::Axis::Z)
					mouseDragPlane.normal.z = 0;
				mouseDragPlane.normal = glm::normalize(mouseDragPlane.normal);
				mouseDragPlane.D = -glm::dot(pos, mouseDragPlane.normal);

				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				mouseDragStart = mouseRay.origin + f * mouseRay.dir;
				mouseDragStart2D = mouseState.position;
				canSelectObject = false;
				originalPos = pos;
			}
			if (g.axisReleased && canSelectObject)
			{
			}
			if (g.axisDragged && canSelectObject)
			{
				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				glm::vec3 mouseOffset = (mouseRay.origin + f * mouseRay.dir) - mouseDragStart;

				if ((g.selectedAxis & Gadget::X) == 0)
					mouseOffset.x = 0;
				if ((g.selectedAxis & Gadget::Y) == 0)
					mouseOffset.y = 0;
				if ((g.selectedAxis & Gadget::Z) == 0)
					mouseOffset.z = 0;

				bool snap = snapToGrid;
				if (ImGui::GetIO().KeyShift)
					snap = !snap;
				if (snap && gridLocal)
					mouseOffset = glm::round(mouseOffset / (float)gridSize) * (float)gridSize;

				float mouseOffset2D = glm::length(mouseDragStart2D - mouseState.position);
				if (snap && gridLocal)
					mouseOffset2D = glm::round(mouseOffset2D / (float)gridSize) * (float)gridSize;
				float poss = glm::sign(mouseState.position.x - mouseDragStart2D.x);
				if (poss == 0)
					poss = 1;

				ImGui::Begin("Statusbar");
				ImGui::SetNextItemWidth(200.0f);
				if (g.mode == Gadget::Mode::Translate || g.mode == Gadget::Mode::Scale)
					ImGui::InputFloat3("Drag Offset", glm::value_ptr(mouseOffset));
				else
					ImGui::Text(std::to_string(poss * mouseOffset2D).c_str());
				ImGui::SameLine();
				ImGui::End();

				if (g.mode == Gadget::Mode::Translate)
				{
					pos = originalPos + mouseOffset;
					if (snap && !gridLocal)
						pos[g.selectedAxisIndex()] = glm::round((pos[g.selectedAxisIndex()] - gridOffset) / (float)gridSize) * (float)gridSize + gridOffset;
				}
				canSelectObject = false;
			}
		}
	}


	fbo->unbind();
}