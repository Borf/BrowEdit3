#include "MapView.h"
#include "Map.h"
#include "BrowEdit.h"
#include "Node.h"
#include "Gadget.h"
#include "components/Rsw.h"
#include "components/Gnd.h"
#include "components/GndRenderer.h"
#include "components/RsmRenderer.h"

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

	//TODO: move this to the select action
	map->rootNode->traverse([&browEdit](Node* n)
	{
			auto rsmRenderer = n->getComponent<RsmRenderer>();
			if (rsmRenderer)
				rsmRenderer->selected = std::find(browEdit->selectedNodes.begin(), browEdit->selectedNodes.end(), n) != browEdit->selectedNodes.end();
	});


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
			Gadget::draw(mouseRay, glm::translate(glm::mat4(1.0f), glm::vec3(5 * gnd->width + avgPos.x, -avgPos.y, -(- 10 - 5 * gnd->height + avgPos.z))));
			glEnable(GL_DEPTH_TEST);

			static std::map<Node*, glm::vec3> originalPositions;

			if (Gadget::axisClicked)
			{
				mouseDragPlane.normal = glm::vec3(0, 1, 0);
				mouseDragPlane.D = avgPos.y;
				
				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				mouseDragStart = mouseRay.origin + f * mouseRay.dir;

				std::cout << "Starting to drag axis" << std::endl;

				originalPositions.clear();
				for (auto n : browEdit->selectedNodes)
					originalPositions[n] = n->getComponent<RswObject>()->position;
				canSelectObject = false;
			}
			else if (Gadget::axisReleased)
			{
				std::cout << "Released axis" << std::endl;
			}

			if (Gadget::axisDragged)
			{
				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				glm::vec3 mouseOffset = (mouseRay.origin + f * mouseRay.dir) - mouseDragStart;

				if ((Gadget::selectedAxis & Gadget::X) == 0)
					mouseOffset.x = 0;
				if ((Gadget::selectedAxis & Gadget::Y) == 0)
					mouseOffset.y = 0;
				if ((Gadget::selectedAxis & Gadget::Z) == 0)
					mouseOffset.z = 0;

				if (ImGui::GetIO().KeyShift)
					mouseOffset = glm::round(mouseOffset/5.0f)*5.0f; //TODO: gridsize

				ImGui::Begin("Debug");
				ImGui::InputFloat3("mouseDrag", glm::value_ptr(mouseOffset));
				ImGui::End();

				for (auto n : originalPositions)
				{
					n.first->getComponent<RswObject>()->position = n.second + mouseOffset * glm::vec3(1,1,-1);
					n.first->getComponent<RsmRenderer>()->setDirty();
				}
				canSelectObject = false;

			}

		}
	}

	if (canSelectObject)
	{
		if (ImGui::IsMouseClicked(0))
		{
			auto collisions = map->rootNode->getCollisions(mouseRay);

			if (collisions.size() > 0)
			{ //TODO: ctrl+click and stuff
				browEdit->selectedNodes.clear();
				browEdit->selectedNodes.push_back(collisions[0].first);
			}
		}
	}

	//glUseProgram(0);
	//glMatrixMode(GL_PROJECTION);
	//glLoadMatrixf(glm::value_ptr(nodeRenderContext.projectionMatrix));
	//glMatrixMode(GL_MODELVIEW);
	//glLoadMatrixf(glm::value_ptr(nodeRenderContext.viewMatrix));
	//glColor3f(1, 0, 0);
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