#include "MapView.h"
#include "Map.h"
#include "BrowEdit.h"
#include "Node.h"
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

MapView::MapView(Map* map, const std::string &viewName) : map(map), viewName(viewName)
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
		int Viewport[4];
		glGetIntegerv(GL_VIEWPORT, Viewport);
		glm::vec2 mousePosScreenSpace(mouseState.position);
		mousePosScreenSpace *= glm::vec2((float)fbo->getWidth() / size.x, (float)fbo->getHeight() / size.y);
		mousePosScreenSpace.y = (Viewport[3] - Viewport[1]) - mousePosScreenSpace.y;
		glm::vec3 retNear = glm::unProject(glm::vec3(mousePosScreenSpace, 0.0f), nodeRenderContext.viewMatrix, nodeRenderContext.projectionMatrix, glm::vec4(Viewport[0], Viewport[1], Viewport[2], Viewport[3]));
		glm::vec3 retFar = glm::unProject(glm::vec3(mousePosScreenSpace, 1.0f), nodeRenderContext.viewMatrix, nodeRenderContext.projectionMatrix, glm::vec4(Viewport[0], Viewport[1], Viewport[2], Viewport[3]));
		math::Ray mouseRay(retNear, glm::normalize(retFar-retNear));

		auto collisions = map->rootNode->getCollisions(mouseRay);

		ImGui::SetNextWindowSize(ImVec2(300, 300));
		ImGui::Begin("Debug");
		ImGui::InputFloat2("mouseState.pos", glm::value_ptr(mouseState.position));
		ImGui::InputFloat2("mousePosScreenSpace", glm::value_ptr(mousePosScreenSpace));
		ImGui::InputFloat3("Origin", glm::value_ptr(mouseRay.origin));
		ImGui::InputFloat3("Dir", glm::value_ptr(mouseRay.dir));
		for (auto c : collisions)
		{
			c.first->getComponent<RsmRenderer>()->selected = true;
			ImGui::Text(c.first->name.c_str());
		}
		ImGui::End();

		if ((mouseState.buttons&4) != 0)
		{
			if (glfwGetKey(browEdit->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) //can I do this with imgui?
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
}