#include "MapView.h"
#include "Map.h"
#include "BrowEdit.h"
#include "Node.h"
#include "Gadget.h"
#include "components/Rsw.h"
#include "components/Gnd.h"
#include "components/GndRenderer.h"
#include "components/RsmRenderer.h"

#include <browedit/actions/ObjectChangeAction.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Vertex.h>
#include <browedit/math/Ray.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <GLFW/glfw3.h>

#include <iostream>

MapView::MapView(Map* map, const std::string &viewName) : map(map), viewName(viewName), mouseRay(glm::vec3(0,0,0), glm::vec3(0,0,0))
{
	fbo = new gl::FBO(1920, 1080, true);
	//shader = new TestShader();

}

void MapView::render(float ratio, float fov)
{
	fbo->bind();
	glViewport(0, 0, fbo->getWidth(), fbo->getHeight());
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (ortho)
		nodeRenderContext.projectionMatrix = glm::ortho(-cameraDistance/2*ratio, cameraDistance/2 * ratio, -cameraDistance/2, cameraDistance/2, -5000.0f, 5000.0f);
	else
		nodeRenderContext.projectionMatrix = glm::perspective(glm::radians(fov), ratio, 0.1f, 5000.0f);
	nodeRenderContext.viewMatrix = glm::mat4(1.0f);
	nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, glm::vec3(0, 0, -cameraDistance));
	nodeRenderContext.viewMatrix = glm::rotate(nodeRenderContext.viewMatrix, glm::radians(cameraRotX), glm::vec3(1, 0, 0));
	nodeRenderContext.viewMatrix = glm::rotate(nodeRenderContext.viewMatrix, glm::radians(cameraRotY), glm::vec3(0, 1, 0));
	nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, glm::vec3(-cameraCenter.x, 0, -cameraCenter.y));

	//TODO: fix this, don't want to individually set settings
	map->rootNode->getComponent<GndRenderer>()->viewLightmapShadow = viewLightmapShadow;
	map->rootNode->getComponent<GndRenderer>()->viewLightmapColor = viewLightmapColor;
	map->rootNode->getComponent<GndRenderer>()->viewColors = viewColors;
	map->rootNode->getComponent<GndRenderer>()->viewLighting = viewLighting;

	RsmRenderer::RsmRenderContext::getInstance()->viewLighting = viewLighting;


	NodeRenderer::render(map->rootNode, nodeRenderContext);
	fbo->unbind();
}



void MapView::update(BrowEdit* browEdit, const ImVec2 &size)
{
	if (fbo->getWidth() != (int)size.x || fbo->getHeight() != (int)size.y)
	{
		std::cout << "FBO is wrong size, resizing..." << std::endl;
		fbo->resize((int)size.x, (int)size.y);
	}
	mouseState.position = glm::vec2(ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x, ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y);
	mouseState.buttons = (ImGui::IsMouseDown(0) ? 0x01 : 0x00) | 
		(ImGui::IsMouseDown(1) ? 0x02 : 0x00) |
		(ImGui::IsMouseDown(2) ? 0x04 : 0x00);

	if (ImGui::IsWindowHovered())
	{
		if (browEdit->editMode == BrowEdit::EditMode::Object)
			updateObjectMode(browEdit);

		if ((mouseState.buttons&4) != 0)
		{
			if (ImGui::GetIO().KeyShift)
			{
				cameraRotX += (mouseState.position.y - prevMouseState.position.y) * 0.25f * browEdit->config.cameraMouseSpeed;
				cameraRotY += (mouseState.position.x - prevMouseState.position.x) * 0.25f * browEdit->config.cameraMouseSpeed;
				cameraRotX = glm::clamp(cameraRotX, 0.0f, 90.0f);
			}
			else
			{
				cameraCenter -= glm::vec2(glm::vec4(
					(mouseState.position.x - prevMouseState.position.x) * browEdit->config.cameraMouseSpeed,
					(mouseState.position.y - prevMouseState.position.y) * browEdit->config.cameraMouseSpeed, 0, 0)
					* glm::rotate(glm::mat4(1.0f), -glm::radians(cameraRotY), glm::vec3(0, 0, 1)));
			}
		}
		cameraDistance *= (1 - (ImGui::GetIO().MouseWheel * 0.1f));
		cameraDistance = glm::clamp(0.0f, 2000.0f, cameraDistance);
	}

	prevMouseState = mouseState;


	int viewPort[4]{ 0, 0, fbo->getWidth(), fbo->getHeight() };
	glm::vec2 mousePosScreenSpace(mouseState.position);
	mousePosScreenSpace.y = fbo->getHeight() - mousePosScreenSpace.y;
	glm::vec3 retNear = glm::unProject(glm::vec3(mousePosScreenSpace, 0.0f), nodeRenderContext.viewMatrix, nodeRenderContext.projectionMatrix, glm::vec4(viewPort[0], viewPort[1], viewPort[2], viewPort[3]));
	glm::vec3 retFar = glm::unProject(glm::vec3(mousePosScreenSpace, 1.0f), nodeRenderContext.viewMatrix, nodeRenderContext.projectionMatrix, glm::vec4(viewPort[0], viewPort[1], viewPort[2], viewPort[3]));
	mouseRay.origin = retNear;
	mouseRay.dir = glm::normalize(retFar - retNear);
	mouseRay.calcSign();
}

void MapView::updateObjectMode(BrowEdit* browEdit)
{

}


void MapView::postRenderObjectMode(BrowEdit* browEdit)
{
	glUseProgram(0);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(nodeRenderContext.projectionMatrix));
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(nodeRenderContext.viewMatrix));
	glColor3f(1, 0, 0);

	fbo->bind();

	bool canSelectObject = true;

	if (browEdit->selectedNodes.size() > 0)
	{
		glm::vec3 avgPos(0.0f);
		int count = 0;
		for(auto n : browEdit->selectedNodes)
			if (n->root == map->rootNode && n->getComponent<RswObject>())
			{
				avgPos += n->getComponent<RswObject>()->position;
				count++;
			}
		if (count > 0)
		{
			avgPos /= count;
			Gnd* gnd = map->rootNode->getComponent<Gnd>();
			Gadget::setMatrices(nodeRenderContext.projectionMatrix, nodeRenderContext.viewMatrix);
			glDisable(GL_DEPTH_TEST);
			gadget.draw(mouseRay, glm::translate(glm::mat4(1.0f), glm::vec3(5 * gnd->width + avgPos.x, -avgPos.y, -(- 10 - 5 * gnd->height + avgPos.z))));
			glEnable(GL_DEPTH_TEST);
			static std::map<Node*, glm::vec3> originalPositions; //TODO: move to action

			if (gadget.axisClicked)
			{
				mouseDragPlane.normal = glm::normalize(glm::vec3(nodeRenderContext.viewMatrix * glm::vec4(0, 0, 1, 1)) - glm::vec3(nodeRenderContext.viewMatrix * glm::vec4(0, 0, 0, 1))) * glm::vec3(1, 1, -1);
				if (gadget.selectedAxis == Gadget::Axis::X)
					mouseDragPlane.normal.x = 0;
				if (gadget.selectedAxis == Gadget::Axis::Y)
					mouseDragPlane.normal.y = 0;
				if (gadget.selectedAxis == Gadget::Axis::Z)
					mouseDragPlane.normal.z = 0;
				mouseDragPlane.normal = glm::normalize(mouseDragPlane.normal);
				mouseDragPlane.D = -glm::dot(glm::vec3(5 * gnd->width + avgPos.x, -avgPos.y, -(-10 - 5 * gnd->height + avgPos.z)), mouseDragPlane.normal);

				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				mouseDragStart = mouseRay.origin + f * mouseRay.dir;

				originalPositions.clear();
				for (auto n : browEdit->selectedNodes)
					originalPositions[n] = n->getComponent<RswObject>()->position;
				canSelectObject = false;
			}
			else if (gadget.axisReleased)
			{
				for (auto n : browEdit->selectedNodes)
				{
					browEdit->doAction(new ObjectChangeAction(n, &n->getComponent<RswObject>()->position, originalPositions[n], "Moving"));
				}
			}

			if (gadget.axisDragged)
			{
				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				glm::vec3 mouseOffset = (mouseRay.origin + f * mouseRay.dir) - mouseDragStart;

				if ((gadget.selectedAxis & Gadget::X) == 0)
					mouseOffset.x = 0;
				if ((gadget.selectedAxis & Gadget::Y) == 0)
					mouseOffset.y = 0;
				if ((gadget.selectedAxis & Gadget::Z) == 0)
					mouseOffset.z = 0;

				bool snap = snapToGrid;
				if (ImGui::GetIO().KeyShift)
					snap = !snap;
				if (snap && gridLocal)
					mouseOffset = glm::round(mouseOffset/(float)gridSize)*(float)gridSize;

				ImGui::Begin("Statusbar");
				ImGui::SetNextItemWidth(200.0f);
				ImGui::InputFloat3("Drag Offset", glm::value_ptr(mouseOffset));
				ImGui::SameLine();
				ImGui::End();

				for (auto n : originalPositions)
				{
					n.first->getComponent<RswObject>()->position = n.second + mouseOffset * glm::vec3(1,-1,-1);
					if (snap && !gridLocal)
						n.first->getComponent<RswObject>()->position[gadget.selectedAxisIndex()] = glm::round(n.first->getComponent<RswObject>()->position[gadget.selectedAxisIndex()] / (float)gridSize) * (float)gridSize;
					n.first->getComponent<RsmRenderer>()->setDirty();
				}
				canSelectObject = false;

			}

		}
	}

	if (canSelectObject && hovered)
	{
		if (ImGui::IsMouseClicked(0))
		{
			auto collisions = map->rootNode->getCollisions(mouseRay);

			if (collisions.size() > 0)
			{
				std::size_t closest = 0;
				float closestDistance = 999999;
				for(std::size_t i = 0; i < collisions.size(); i++)
					for(const auto &pos : collisions[i].second)
						if (glm::distance(mouseRay.origin, pos) < closestDistance)
						{
							closest = i;
							closestDistance = glm::distance(mouseRay.origin, pos) < closestDistance;
						}
				browEdit->doAction(new SelectAction(browEdit, collisions[closest].first, ImGui::GetIO().KeyShift, std::find(browEdit->selectedNodes.begin(), browEdit->selectedNodes.end(), collisions[closest].first) != browEdit->selectedNodes.end() && ImGui::GetIO().KeyShift));
			}
		}
	}

	//glBegin(GL_LINES);
	//glVertex3f(0, 0, 0);
	//glVertex3f(100, 0, 0);
	//glVertex3f(0, 0, 0);
	//glVertex3f(0, 100, 0);
	//glVertex3f(0, 0, 0);
	//glVertex3f(0, 0, 100);
	//glEnd();

	fbo->unbind();
}