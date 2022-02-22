#include "MapView.h"

#include <browedit/Components/Gnd.h>
#include <browedit/Components/Rsw.h>
#include <browedit/BrowEdit.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/VBO.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/NewObjectAction.h>
#include <browedit/actions/ObjectChangeAction.h>
#include <browedit/shaders/SimpleShader.h>

#include <glm/gtc/type_ptr.hpp>
#include <mutex>

extern std::vector<std::vector<glm::vec3>> debugPoints;
extern std::mutex debugPointMutex;


void MapView::postRenderObjectMode(BrowEdit* browEdit)
{
	simpleShader->use();
	simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
	simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);


	glUseProgram(0);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(nodeRenderContext.projectionMatrix));
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(nodeRenderContext.viewMatrix));
	glColor3f(1, 0, 0);
	fbo->bind();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_2D);


	glPushMatrix();
	glScalef(1, -1, -1);
	auto gnd = map->rootNode->getComponent<Gnd>();
	glTranslatef(gnd->width * 5.0f, 0.0f, -gnd->height * 5.0f);
	map->rootNode->getComponent<Rsw>()->quadtree->draw(quadTreeMaxLevel);
	glPopMatrix();

	static glm::vec4 color[] = { glm::vec4(1,0,0,1), glm::vec4(0,1,0,1), glm::vec4(0,0,1,1) };
	debugPointMutex.lock();
	glPointSize(10.0f);
	for (auto i = 0; i < debugPoints.size(); i++)
	{
		glColor4fv(glm::value_ptr(color[i]));
		glBegin(GL_POINTS);
		for (auto& v : debugPoints[i])
			glVertex3fv(glm::value_ptr(v));
		glEnd();
	}
	glPointSize(1.0f);
	debugPointMutex.unlock();

	bool snap = snapToGrid;
	if (ImGui::GetIO().KeyShift)
		snap = !snap;
	if (map->selectedNodes.size() > 0 && snap)
	{
		simpleShader->use();
		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0, 0, 0, 1));

		glm::vec3 avgPos = map->getSelectionCenter();
		avgPos.y -= 0.1f;
		if (!gridLocal)
		{
			avgPos.x = glm::floor(avgPos.x / gridSize) * gridSize;
			avgPos.z = glm::floor(avgPos.z / gridSize) * gridSize;
		}

		glm::mat4 mat = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, -1));
		mat = glm::translate(mat, glm::vec3(5 * gnd->width + avgPos.x, -avgPos.y, (-10 - 5 * gnd->height + avgPos.z)));

//		glDisable(GL_DEPTH_TEST);
		simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, mat);
		glLineWidth(2);
		gridVbo->bind();
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4); //TODO: vao
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(0 * sizeof(float)));
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(3 * sizeof(float)));
		glDrawArrays(GL_LINES, 0, (int)gridVbo->size());
//		glEnable(GL_DEPTH_TEST);
		gridVbo->unBind();
	}


	if (showAllLights)
		map->rootNode->traverse([&](Node* n)
			{
				auto rswLight = n->getComponent<RswLight>();
				if (rswLight)
					drawLight(n);
			});


	if (hovered && ImGui::IsMouseReleased(1) && ImGui::GetIO().MouseDragMaxDistanceSqr[1] < 5)
		ImGui::OpenPopup("Object Rightclick");
	if (ImGui::BeginPopup("Object Rightclick"))
	{
		if (ImGui::MenuItem("Set to floor height"))
			map->setSelectedItemsToFloorHeight(browEdit);
		if (ImGui::MenuItem("Copy", "Ctrl+c"))
			map->copySelection();
		if (ImGui::MenuItem("Paste", "Ctrl+v"))
			map->pasteSelection(browEdit);
		if (ImGui::MenuItem("Select Same"))
			map->selectSameModels(browEdit);
		static float distance = 50.0f;
		ImGui::PushItemWidth(75.0f);
		ImGui::DragFloat("Near Distance", &distance, 1.0f, 0.0f, 1000.0f);
		if (ImGui::MenuItem("Select near"))
			map->selectNear(distance, browEdit);
		ImGui::EndPopup();
	}


	bool canSelectObject = true;
	if (browEdit->newNodes.size() > 0 && hovered)
	{
		if (ImGui::IsMouseClicked(0))
		{
			auto ga = new GroupAction("Pasting " + std::to_string(browEdit->newNodes.size()) + " objects");
			bool first = false;
			for (auto newNode : browEdit->newNodes)
			{
				std::string path = newNode.first->name;
				if (path.find("\\") != std::string::npos)
					path = path.substr(0, path.find("\\"));
				newNode.first->setParent(map->findAndBuildNode(path));
				
				bool exists = false;
				auto& siblings = newNode.first->parent->children;
				for (auto s : siblings)
					if (s->name == newNode.first->name && s != newNode.first)
						exists = true;

				if (exists)
				{
					std::string name = newNode.first->name;
					int index = 0;
					exists = true;
					while (exists)
					{
						index++;
						newNode.first->name = name + "_" + std::string(3 - std::min(3, (int)std::to_string(index).length()), '0') + std::to_string(index);
						exists = false;
						for (auto s : siblings)
							if (s->name == newNode.first->name && s != newNode.first)
								exists = true;
					}
				}

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
		for (auto n : map->selectedNodes)
			if (n->root == map->rootNode && n->getComponent<RswObject>())
			{
				avgPos += n->getComponent<RswObject>()->position;
				count++;
			}
		if (count > 0)
		{
			if (!showAllLights)
				for (auto n : map->selectedNodes)
				{
					auto rswLight = n->getComponent<RswLight>();
					if (rswLight)
						drawLight(n);
				}

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
			if (hovered)
			{
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
								originalAngle -= glm::radians(-pos * mouseOffset2D);
								if (snap && !gridLocal)
									originalAngle = glm::radians(glm::round(glm::degrees(originalAngle) / (float)gridSize) * (float)gridSize);

								n.first->getComponent<RswObject>()->position.x = groupCenter.x + dist * glm::cos(originalAngle);
								n.first->getComponent<RswObject>()->position.z = groupCenter.z + dist * glm::sin(originalAngle);
							}
							if (snap && !gridLocal)
								n.first->getComponent<RswObject>()->rotation[gadget.selectedAxisIndex()] = glm::round(n.first->getComponent<RswObject>()->rotation[gadget.selectedAxisIndex()] / (float)gridSize) * (float)gridSize;
						}
						if (n.first->getComponent<RsmRenderer>())
							n.first->getComponent<RsmRenderer>()->setDirty();
					}
					canSelectObject = false;
				}
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
				for (std::size_t i = 0; i < collisions.size(); i++)
					for (const auto& pos : collisions[i].second)
						if (glm::distance(mouseRay.origin, pos) < closestDistance)
						{
							closest = i;
							closestDistance = glm::distance(mouseRay.origin, pos);
						}
				map->doAction(new SelectAction(map, collisions[closest].first, ImGui::GetIO().KeyShift, std::find(map->selectedNodes.begin(), map->selectedNodes.end(), collisions[closest].first) != map->selectedNodes.end() && ImGui::GetIO().KeyShift), browEdit);
			}
		}
	}
	fbo->unbind();
}


 

void MapView::rebuildObjectModeGrid()
{
	std::vector<VertexP3T2> verts;
	if (gridSize > 0)
	{
		for (float i = -10 * gridSize; i <= 10 * gridSize; i += gridSize)
		{
			verts.push_back(VertexP3T2(glm::vec3(-10 * gridSize, 0, i), glm::vec2(0)));
			verts.push_back(VertexP3T2(glm::vec3(10 * gridSize, 0, i), glm::vec2(0)));

			verts.push_back(VertexP3T2(glm::vec3(i, 0, -10 * gridSize), glm::vec2(0)));
			verts.push_back(VertexP3T2(glm::vec3(i, 0, 10 * gridSize), glm::vec2(0)));
		}
	}
	gridVbo->setData(verts, GL_STATIC_DRAW);
}