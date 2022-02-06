#include "MapView.h"
#include "Map.h"
#include "BrowEdit.h"
#include "Node.h"
#include "Gadget.h"
#include "components/Rsw.h"
#include "components/Gnd.h"
#include "components/GndRenderer.h"
#include "components/RsmRenderer.h"

#include <browedit/actions/GroupAction.h>
#include <browedit/actions/ObjectChangeAction.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/NewObjectAction.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Vertex.h>
#include <browedit/math/Ray.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <GLFW/glfw3.h>

#include <iostream>

MapView::MapView(Map* map, const std::string &viewName) : map(map), viewName(viewName), mouseRay(glm::vec3(0,0,0), glm::vec3(0,0,0))
{
	fbo = new gl::FBO(1920, 1080, true);
	//shader = new TestShader();

	auto gnd = map->rootNode->getComponent<Gnd>();
	if (gnd)
	{
		cameraCenter.x = gnd->width * 5.0f;
		cameraCenter.y = gnd->height * 5.0f;
	}
}

void MapView::toolbar(BrowEdit* browEdit)
{
	browEdit->toolBarToggleButton("ortho", ortho ? 24 : 25, &ortho, "Toggle between ortho and perspective camera");
	ImGui::SameLine();

	browEdit->toolBarToggleButton("viewLightMapShadow", 8, &viewLightmapShadow, "Toggle shadowmap");
	ImGui::SameLine();
	browEdit->toolBarToggleButton("viewLightmapColor", 9, &viewLightmapColor, "Toggle colormap");
	ImGui::SameLine();
	browEdit->toolBarToggleButton("viewColors", 11, &viewColors, "Toggle tile colors");
	ImGui::SameLine();
	browEdit->toolBarToggleButton("viewLighting", 12, &viewLighting, "Toggle lighting");
	ImGui::SameLine();
	browEdit->toolBarToggleButton("smoothColors", 63, &smoothColors, "Smooth colormap");
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragInt("##quadTreeMaxLevel", &quadTreeMaxLevel, 1, 0, 6);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Change quadtree detail level");
	ImGui::SameLine();


	bool snapping = snapToGrid;
	if (ImGui::GetIO().KeyShift)
		snapping = !snapping;
	bool ret = browEdit->toolBarToggleButton("snapToGrid", 14, snapping, "Snap to grid");
	if (!ImGui::GetIO().KeyShift && ret)
		snapToGrid = !snapToGrid;
	if (snapping || snapToGrid)
	{
		ImGui::SameLine();
		ImGui::SetNextItemWidth(50);
		ImGui::DragFloat("##gridSize", &gridSize, 1.0f, 0.1f, 100.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Grid size. Doubleclick or ctrl+click to type a number");
		ImGui::SameLine();
		ImGui::Checkbox("##gridLocal", &gridLocal);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Local or Global grid. Either makes the movement rounded off, or the final position");
	}


	if (browEdit->editMode == BrowEdit::EditMode::Object)
	{
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("translate", 20, gadget.mode == Gadget::Mode::Translate, "Move"))
			gadget.mode = Gadget::Mode::Translate;
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("rotate", 21, gadget.mode == Gadget::Mode::Rotate, "Rotate"))
			gadget.mode = Gadget::Mode::Rotate;
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("scale", 22, gadget.mode == Gadget::Mode::Scale, "Scale"))
			gadget.mode = Gadget::Mode::Scale;


		if (gadget.mode == Gadget::Mode::Rotate || gadget.mode == Gadget::Mode::Scale)
		{
			ImGui::SameLine();
			if (browEdit->toolBarToggleButton("localglobal", pivotPoint == MapView::PivotPoint::Local ? MISSING : MISSING, false, "Changes the pivot point for rotations"))
			{
				if (pivotPoint == MapView::PivotPoint::Local)
					pivotPoint = MapView::PivotPoint::GroupCenter;
				else
					pivotPoint = MapView::PivotPoint::Local;
			}

		}
	}
}

void MapView::render(BrowEdit* browEdit)
{
	fbo->bind();
	glViewport(0, 0, fbo->getWidth(), fbo->getHeight());
	glClearColor(browEdit->config.backgroundColor.r, browEdit->config.backgroundColor.g, browEdit->config.backgroundColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	float ratio = fbo->getWidth() / (float)fbo->getHeight();

	if (ortho)
		nodeRenderContext.projectionMatrix = glm::ortho(-cameraDistance/2*ratio, cameraDistance/2 * ratio, -cameraDistance/2, cameraDistance/2, -5000.0f, 5000.0f);
	else
		nodeRenderContext.projectionMatrix = glm::perspective(glm::radians(browEdit->config.fov), ratio, 0.1f, 5000.0f);
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
	if (map->rootNode->getComponent<GndRenderer>()->smoothColors != smoothColors)
	{
		map->rootNode->getComponent<GndRenderer>()->smoothColors = smoothColors;
		map->rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
	}

	RsmRenderer::RsmRenderContext::getInstance()->viewLighting = viewLighting;


	NodeRenderer::render(map->rootNode, nodeRenderContext);

	if (browEdit->newNodes.size() > 0)
	{
		for (auto newNode : browEdit->newNodes)
		{
			auto rswObject = newNode.first->getComponent<RswObject>();
			auto gnd = map->rootNode->getComponent<Gnd>();
			auto rayCast = gnd->rayCast(mouseRay);
			rswObject->position = glm::vec3(rayCast.x - 5 * gnd->width, -rayCast.y, -(rayCast.z + (-10 - 5 * gnd->height))) + newNode.second;
			newNode.first->getComponent<RsmRenderer>()->matrixCache = glm::translate(glm::mat4(1.0f), rayCast + newNode.second);
			newNode.first->getComponent<RsmRenderer>()->matrixCache = glm::scale(newNode.first->getComponent<RsmRenderer>()->matrixCache, glm::vec3(1, -1, 1));
			NodeRenderer::render(newNode.first, nodeRenderContext);
		}
	}

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

	if (hovered)
	{
		if (browEdit->editMode == BrowEdit::EditMode::Object)
			updateObjectMode(browEdit);

		if ((mouseState.buttons&6) != 0)
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

	glPushMatrix();
	glScalef(1, -1, -1);
	auto gnd = map->rootNode->getComponent<Gnd>();
	glTranslatef(gnd->width * 5.0f, 0.0f, -gnd->height * 5.0f);
	map->rootNode->getComponent<Rsw>()->quadtree->draw(quadTreeMaxLevel);
	glPopMatrix();


	if (!hovered)
	{
		fbo->unbind();
		return;
	}


	bool canSelectObject = true;
	if (browEdit->newNodes.size() > 0)
	{
		if (ImGui::IsMouseClicked(0))
		{
			auto ga = new GroupAction("Pasting " + std::to_string(browEdit->newNodes.size()) + " objects");
			bool first = false;
			for (auto newNode : browEdit->newNodes)
			{
				newNode.first->setParent(map->rootNode);
				ga->addAction(new NewObjectAction(newNode.first));
				auto sa = new SelectAction(map, newNode.first, first, false);
				sa->perform(map, browEdit); // to make sure everything is selected
				ga->addAction(sa);
				first = true;
			}
			map->doAction(ga, browEdit);
			browEdit->newNodes.clear();
		}
	}
	else if (map->selectedNodes.size() > 0)
	{
		glm::vec3 avgPos(0.0f);
		int count = 0;
		for(auto n : map->selectedNodes)
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
			glClear(GL_DEPTH_BUFFER_BIT);
			glDisable(GL_CULL_FACE);

			glm::mat4 mat = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, -1));
			mat = glm::translate(mat, glm::vec3(5 * gnd->width + avgPos.x, -avgPos.y, (-10 - 5 * gnd->height + avgPos.z)));
			if (map->selectedNodes.size() == 1 && gadget.mode == Gadget::Mode::Rotate)
			{
				mat = glm::rotate(mat, glm::radians(map->selectedNodes[0]->getComponent<RswObject>()->rotation.y), glm::vec3(0, 1, 0));
				mat = glm::rotate(mat, -glm::radians(map->selectedNodes[0]->getComponent<RswObject>()->rotation.x), glm::vec3(1, 0, 0));
				mat = glm::rotate(mat, -glm::radians(map->selectedNodes[0]->getComponent<RswObject>()->rotation.z), glm::vec3(0, 0, 1));
			}

			gadget.draw(mouseRay, mat);
			struct PosRotScale {
				glm::vec3 pos;
				glm::vec3 rot;
				glm::vec3 scale;
				PosRotScale(RswObject* o) : pos(o->position), rot(o->rotation), scale(o->scale) {}
				PosRotScale() : pos(0.0f), rot(0.0f), scale(1.0f) {}
			};
			static std::map<Node*, PosRotScale> originalValues;
			static glm::vec3 groupCenter;

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
				mouseDragStart2D = mouseState.position;

				groupCenter = glm::vec3(0.0f);
				originalValues.clear();
				int count = 0;
				for (auto n : map->selectedNodes)
				{
					auto rswObject = n->getComponent<RswObject>();
					if (rswObject)
					{
						originalValues[n] = PosRotScale(rswObject);
						groupCenter += n->getComponent<RswObject>()->position;
						count++;
					}
				}
				groupCenter /= count;
				canSelectObject = false;
			}
			else if (gadget.axisReleased)
			{
				GroupAction* ga = new GroupAction();
				for (auto n : map->selectedNodes)
				{ //TODO: 
					if (gadget.mode == Gadget::Mode::Translate)
						ga->addAction(new ObjectChangeAction(n, &n->getComponent<RswObject>()->position, originalValues[n].pos, "Moving"));
					else if (gadget.mode == Gadget::Mode::Scale)
						ga->addAction(new ObjectChangeAction(n, &n->getComponent<RswObject>()->scale, originalValues[n].scale, "Scaling"));
					else if (gadget.mode == Gadget::Mode::Rotate)
					{
						ga->addAction(new ObjectChangeAction(n, &n->getComponent<RswObject>()->rotation, originalValues[n].rot, "Rotating"));
						ga->addAction(new ObjectChangeAction(n, &n->getComponent<RswObject>()->position, originalValues[n].pos, ""));
					}
				}
				map->doAction(ga, browEdit);
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
					mouseOffset = glm::round(mouseOffset / (float)gridSize) * (float)gridSize;

				float mouseOffset2D = glm::length(mouseDragStart2D - mouseState.position);
				if (snap && gridLocal)
					mouseOffset2D = glm::round(mouseOffset2D / (float)gridSize) * (float)gridSize;
				float pos = glm::sign(mouseState.position.x - mouseDragStart2D.x);
				if (pos == 0)
					pos = 1;

				ImGui::Begin("Statusbar");
				ImGui::SetNextItemWidth(200.0f);
				if (gadget.mode == Gadget::Mode::Translate || gadget.mode == Gadget::Mode::Scale)
					ImGui::InputFloat3("Drag Offset", glm::value_ptr(mouseOffset));
				else
					ImGui::Text(std::to_string(pos * mouseOffset2D).c_str());
				ImGui::SameLine();
				ImGui::End();

				for (auto n : originalValues)
				{
					if (gadget.mode == Gadget::Mode::Translate)
					{
						n.first->getComponent<RswObject>()->position = n.second.pos + mouseOffset * glm::vec3(1, -1, -1);
						if (snap && !gridLocal)
							n.first->getComponent<RswObject>()->position[gadget.selectedAxisIndex()] = glm::round(n.first->getComponent<RswObject>()->position[gadget.selectedAxisIndex()] / (float)gridSize) * (float)gridSize;
					}
					if (gadget.mode == Gadget::Mode::Scale)
					{
						if (gadget.selectedAxis == (Gadget::Axis::X | Gadget::Axis::Y | Gadget::Axis::Z))
						{
							n.first->getComponent<RswObject>()->scale = n.second.scale * (1 + pos * glm::length(0.01f * mouseOffset));
							if (snap && !gridLocal)
								n.first->getComponent<RswObject>()->scale = glm::round(n.first->getComponent<RswObject>()->scale / (float)gridSize) * (float)gridSize;
						}
						else
						{
							n.first->getComponent<RswObject>()->scale[gadget.selectedAxisIndex()] = n.second.scale[gadget.selectedAxisIndex()] * (1 + pos * glm::length(0.01f * mouseOffset));
							if (snap && !gridLocal)
								n.first->getComponent<RswObject>()->scale[gadget.selectedAxisIndex()] = glm::round(n.first->getComponent<RswObject>()->scale[gadget.selectedAxisIndex()] / (float)gridSize) * (float)gridSize;
						}
					}
					if (gadget.mode == Gadget::Mode::Rotate)
					{
						n.first->getComponent<RswObject>()->rotation[gadget.selectedAxisIndex()] = n.second.rot[gadget.selectedAxisIndex()] + -pos * mouseOffset2D;
						if (pivotPoint == PivotPoint::GroupCenter)
						{
							float originalAngle = atan2(n.second.pos.z - groupCenter.z, n.second.pos.x - groupCenter.x);
							float dist = glm::length(glm::vec2(n.second.pos.z - groupCenter.z, n.second.pos.x - groupCenter.x));
							originalAngle -= glm::radians(- pos * mouseOffset2D);
							if (snap && !gridLocal)
								originalAngle = glm::radians(glm::round(glm::degrees(originalAngle) / (float)gridSize) * (float)gridSize);

							n.first->getComponent<RswObject>()->position.x = groupCenter.x + dist * glm::cos(originalAngle);
							n.first->getComponent<RswObject>()->position.z = groupCenter.z + dist * glm::sin(originalAngle);
						}
						if (snap && !gridLocal)
							n.first->getComponent<RswObject>()->rotation[gadget.selectedAxisIndex()] = glm::round(n.first->getComponent<RswObject>()->rotation[gadget.selectedAxisIndex()] / (float)gridSize) * (float)gridSize;
					}
					if(n.first->getComponent<RsmRenderer>())
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
				map->doAction(new SelectAction(map, collisions[closest].first, ImGui::GetIO().KeyShift, std::find(map->selectedNodes.begin(), map->selectedNodes.end(), collisions[closest].first) != map->selectedNodes.end() && ImGui::GetIO().KeyShift), browEdit);
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



void MapView::focusSelection()
{

}

