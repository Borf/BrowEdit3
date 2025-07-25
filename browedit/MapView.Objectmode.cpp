#include <Windows.h>
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
#include <browedit/math/Polygon.h>
#include <browedit/HotkeyRegistry.h>

#include <glm/gtc/type_ptr.hpp>
#include <mutex>

extern std::vector<std::vector<glm::vec3>> debugPoints;


void MapView::postRenderObjectMode(BrowEdit* browEdit)
{
	float gridSize = gridSizeTranslate;
	float gridOffset = gridOffsetTranslate;
	if (gadget.mode == Gadget::Mode::Rotate)
	{
		gridSize = gridSizeRotate;
		gridOffset = gridOffsetRotate;
	}
	else if (gadget.mode == Gadget::Mode::Scale)
	{
		gridSize = gridSizeScale;
		gridOffset = gridOffsetScale;
	}
	simpleShader->use();
	simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
	simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);


	fbo->bind();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_2D);

	auto gnd = map->rootNode->getComponent<Gnd>();

#if 0 //visualize aabb
	glEnable(GL_BLEND);
	glColor4f(1, 0, 0, 0.3f);
	glBegin(GL_TRIANGLES);
	map->rootNode->traverse([](Node* n) {
		auto rswModel = n->getComponent<RswModel>();
		if (rswModel)
		{
			auto verts = math::AABB::box(rswModel->aabb.min, rswModel->aabb.max);
			for (auto& v : verts)
				glVertex3f(v.x, v.y, v.z);
		}
	});
	glEnd();
#endif

	bool snap = snapToGrid;
	if (ImGui::GetIO().KeyShift)
		snap = !snap;
	
	if (map->selectedNodes.size() > 0 && snap)
	{
		simpleShader->use();
		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0, 0, 0, 1));

		// The rotate grid is drawn later instead
		if (gadget.mode != Gadget::Mode::Rotate) {
			glm::vec3 avgPos = map->getSelectionCenter();
			avgPos.y -= 0.1f;
			if (!gridLocal)
			{
				avgPos.x = glm::floor((avgPos.x-gridOffset) / gridSize) * gridSize + gridOffset;
				avgPos.z = glm::floor((avgPos.z-gridOffset) / gridSize) * gridSize + gridOffset;
			}

			glm::mat4 mat = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, -1));
			mat = glm::translate(mat, glm::vec3(5 * gnd->width + avgPos.x, -avgPos.y, (-10 - 5 * gnd->height + avgPos.z)));

			//glDisable(GL_DEPTH_TEST);
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
			//glEnable(GL_DEPTH_TEST);
			gridVbo->unBind();
		}
	}
	if (objectSelectLasso.size() > 0)
	{
		float dist = 0.002f * cameraDistance;
		simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
		glLineWidth(5);
		std::vector<VertexP3T2N3> verts;
		for(auto& v : objectSelectLasso)
			verts.push_back(VertexP3T2N3(v + glm::vec3(0, dist, 0), glm::vec2(0), glm::vec3(1.0f)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4); //TODO: vao
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);

		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 0, 1));
		glDrawArrays(GL_LINE_LOOP, 0, (int)verts.size());
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
		browEdit->hotkeyMenuItem("Copy", HotkeyAction::Global_Copy);
		browEdit->hotkeyMenuItem("Paste", HotkeyAction::Global_Paste);
		if (ImGui::MenuItem("Select Same"))
			map->selectSameModels(browEdit);
		static float distance = 50.0f;
		ImGui::PushItemWidth(75.0f);
		ImGui::DragFloat("Near Distance", &distance, 1.0f, 0.0f, 1000.0f);
		if (ImGui::MenuItem("Select near"))
			map->selectNear(distance, browEdit);
		browEdit->hotkeyMenuItem("Create Prefab", HotkeyAction::ObjectEdit_CreatePrefab);
		browEdit->hotkeyMenuItem("Highlight in Object Picker", HotkeyAction::ObjectEdit_HighlightInObjectPicker);


		ImGui::EndPopup();
	}
	if (browEdit->windowData.openPrefabPopup)
	{
		ImGui::OpenPopup("CreatePrefab");
		browEdit->windowData.openPrefabPopup = false;
	}
	if (ImGui::BeginPopup("CreatePrefab"))
	{
		static char fileName[1000] = "prefab";
		ImGui::InputText("Filename", fileName, 1000);
		if (ImGui::Button("Create"))
		{
			map->createPrefab(std::string(fileName) + ".json", browEdit);
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}


	bool canSelectObject = true;
	if (browEdit->newNodes.size() > 0 && hovered)
	{
		if (ImGui::IsMouseClicked(0))
		{
			canSelectObject = false;
			auto ga = new GroupAction("Pasting " + std::to_string(browEdit->newNodes.size()) + " objects");
			bool first = false;
			for (auto newNode : browEdit->newNodes)
			{
				std::string path = newNode.first->name;
				if (path.find("\\") != std::string::npos)
					path = path.substr(0, path.rfind("\\"));
				else
					path = "";
				newNode.first->setParent(map->findAndBuildNode(path));
				newNode.first->makeNameUnique(map->rootNode);

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

			glm::mat4 viewMatrix = glm::translate(nodeRenderContext.viewMatrix, cameraCenter);
			Gadget::setMatrices(nodeRenderContext.projectionMatrix, nodeRenderContext.viewMatrix, -glm::normalize(viewMatrix[3]));
			glClear(GL_DEPTH_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			glEnable(GL_BLEND);

			glm::mat4 rotMat = glm::mat4(1.0f);
			glm::mat4 mat = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, -1));
			mat = glm::translate(mat, glm::vec3(5 * gnd->width + avgPos.x, -avgPos.y, (-10 - 5 * gnd->height + avgPos.z)));
			
			if (!(gadget.axisClicked || gadget.axisDragged)) {
				lockedGizmo = !ImGui::GetIO().KeyCtrl;
			}

			auto rotVector = map->selectedNodes[0]->getComponent<RswObject>() ? &map->selectedNodes[0]->getComponent<RswObject>()->rotation : nullptr;

			if (rotVector) {
				if (!lockedGizmo) {
					if (map->selectedNodes.size() == 1 && gadget.mode == Gadget::Mode::Rotate) {
						mat = glm::rotate(mat, -glm::radians(rotVector->z), glm::vec3(0, 0, 1));
						rotMat = glm::rotate(rotMat, -glm::radians(rotVector->z), glm::vec3(0, 0, 1));
					}
				}

				if (map->selectedNodes.size() == 1 && gadget.mode == Gadget::Mode::Scale) {
					mat = glm::rotate(mat, -glm::radians(rotVector->z), glm::vec3(0, 0, 1));
					mat = glm::rotate(mat, -glm::radians(rotVector->x), glm::vec3(1, 0, 0));
					mat = glm::rotate(mat, glm::radians(rotVector->y), glm::vec3(0, 1, 0));

					rotMat = glm::rotate(rotMat, -glm::radians(rotVector->z), glm::vec3(0, 0, 1));
					rotMat = glm::rotate(rotMat, glm::radians(rotVector->x), glm::vec3(1, 0, 0));
					rotMat = glm::rotate(rotMat, glm::radians(rotVector->y), glm::vec3(0, 1, 0));
				}
			}

			gadget.draw(mouseRay, mat, !lockedGizmo && map->selectedNodes[0]->getComponent<RswObject>() ? -rotVector->x : 0);

			if (!lockedGizmo && map->selectedNodes[0]->getComponent<RswObject>() && map->selectedNodes.size() == 1 && gadget.mode == Gadget::Mode::Rotate) {
				if (gadget.selectedAxis == Gadget::Axis::Y) {
					rotMat = glm::rotate(rotMat, glm::radians(rotVector->x), glm::vec3(1, 0, 0));
				}
			}

			struct PosRotScale {
				glm::vec3 pos;
				glm::vec3 rot;
				glm::vec3 scale;
				PosRotScale(RswObject* o) : pos(o->position), rot(o->rotation), scale(o->scale) {}
				PosRotScale() : pos(0.0f), rot(0.0f), scale(1.0f) {}
			};
			static std::map<Node*, PosRotScale> originalValues;
			static glm::vec3 groupCenter;

			static glm::vec3 centerGadget;
			static glm::vec3 vectorStart;
			
			if (hovered || gadget.axisHovered)
			{
				if (gadget.axisClicked)
				{
					mouseDragPlane.normal = glm::normalize(glm::vec3(nodeRenderContext.viewMatrix * glm::vec4(0, 0, 1, 1)) - glm::vec3(nodeRenderContext.viewMatrix * glm::vec4(0, 0, 0, 1))) * glm::vec3(1, 1, -1);
					
					if (gadget.mode == Gadget::Mode::Rotate) {
						if (gadget.selectedAxis == Gadget::Axis::X)
							mouseDragPlane.normal = glm::vec3(1, 0, 0);
						else if (gadget.selectedAxis == Gadget::Axis::Y)
							mouseDragPlane.normal = glm::vec3(0, 1, 0);
						else if (gadget.selectedAxis == Gadget::Axis::Z)
							mouseDragPlane.normal = glm::vec3(0, 0, 1);

						mouseDragPlane.normal = rotMat * glm::vec4(mouseDragPlane.normal, 1);
					}
					else if (gadget.mode == Gadget::Mode::Scale) {
						if (gadget.selectedAxis == Gadget::Axis::X)
							mouseDragPlane.normal = glm::vec3(0, 1, 0);
						else if (gadget.selectedAxis == Gadget::Axis::Y)
							mouseDragPlane.normal = glm::vec3(0, 0, 1);
						else if (gadget.selectedAxis == Gadget::Axis::Z)
							mouseDragPlane.normal = glm::vec3(0, 1, 0);

						mouseDragPlane.normal = rotMat * glm::vec4(mouseDragPlane.normal, 1);
					}
					else {
						if (gadget.selectedAxis == Gadget::Axis::X)
							mouseDragPlane.normal.x = 0;
						if (gadget.selectedAxis == Gadget::Axis::Y)
							mouseDragPlane.normal.y = 0;
						if (gadget.selectedAxis == Gadget::Axis::Z)
							mouseDragPlane.normal.z = 0;
					}

					mouseDragPlane.normal = glm::normalize(mouseDragPlane.normal);
					mouseDragPlane.D = -glm::dot(glm::vec3(5 * gnd->width + avgPos.x, -avgPos.y, -(-10 - 5 * gnd->height + avgPos.z)), mouseDragPlane.normal);

					float f;
					mouseRay.planeIntersection(mouseDragPlane, f);
					mouseDragStart = mouseRay.origin + f * mouseRay.dir;
					mouseDragStart2D = mouseState.position;

					centerGadget = glm::vec3(5 * gnd->width + avgPos.x, -avgPos.y, -(-10 - 5 * gnd->height + avgPos.z));
					vectorStart = (mouseRay.origin + f * mouseRay.dir) - centerGadget;

					if (gadget.mode == Gadget::Mode::Rotate || gadget.mode == Gadget::Mode::Scale) {
						if (gadget.selectedAxis == Gadget::Axis::X) {
							vectorStart = glm::vec4(vectorStart, 1) * rotMat;
						}
						else if (gadget.selectedAxis == Gadget::Axis::Y) {
							vectorStart = glm::vec4(vectorStart, 1) * rotMat;
						}
						else if (gadget.selectedAxis == Gadget::Axis::Z) {
							vectorStart = glm::vec4(vectorStart, 1);
						}
					}

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
						auto rswObject = n->getComponent<RswObject>();
						if (!rswObject)
							continue;
						if (gadget.mode == Gadget::Mode::Translate)
							ga->addAction(new ObjectChangeAction(n, &rswObject->position, originalValues[n].pos, "Moving"));
						else if (gadget.mode == Gadget::Mode::Scale) {
							ga->addAction(new ObjectChangeAction(n, &rswObject->scale, originalValues[n].scale, "Scaling"));
							ga->addAction(new ObjectChangeAction(n, &rswObject->position, originalValues[n].pos, ""));
						}
						else if (gadget.mode == Gadget::Mode::Rotate)
						{
							ga->addAction(new ObjectChangeAction(n, &rswObject->rotation, originalValues[n].rot, "Rotating"));
							ga->addAction(new ObjectChangeAction(n, &rswObject->position, originalValues[n].pos, ""));
						}
					}
					map->doAction(ga, browEdit);
				}

				if (gadget.axisDragged)
				{
					float f = 9999;
					mouseRay.planeIntersection(mouseDragPlane, f);
					glm::vec3 mouseOffset = (mouseRay.origin + f * mouseRay.dir);

					float angle = 0;
					float scale = 0;

					if ((gadget.mode == Gadget::Mode::Rotate || gadget.mode == Gadget::Mode::Scale) && f < 0) {
						// Reverse mouse position
						mouseOffset = mouseOffset - centerGadget;
						mouseOffset *= -1.0f;
						mouseOffset = mouseOffset + centerGadget;
					}

					if (gadget.mode == Gadget::Mode::Rotate) {
						glm::vec3 vectorEnd = mouseOffset - centerGadget;

						if (gadget.selectedAxis == Gadget::Axis::X) {
							vectorEnd = glm::vec4(vectorEnd, 1) * rotMat;
							angle = atan2(vectorEnd.z * vectorStart.y - vectorEnd.y * vectorStart.z, vectorEnd.y * vectorStart.y + vectorEnd.z * vectorStart.z);
						}
						else if (gadget.selectedAxis == Gadget::Axis::Y) {
							vectorEnd = glm::vec4(vectorEnd, 1) * rotMat;
							angle = atan2(vectorEnd.z * vectorStart.x - vectorEnd.x * vectorStart.z, vectorEnd.x * vectorStart.x + vectorEnd.z * vectorStart.z);
						}
						else if (gadget.selectedAxis == Gadget::Axis::Z) {
							angle = -atan2(vectorEnd.y * vectorStart.x - vectorEnd.x * vectorStart.y, vectorEnd.x * vectorStart.x + vectorEnd.y * vectorStart.y);
						}
					}
					else if (gadget.mode == Gadget::Mode::Scale) {
						glm::vec3 vectorEnd = mouseOffset - centerGadget;

						if (gadget.selectedAxis == Gadget::Axis::X) {
							vectorEnd = glm::vec4(vectorEnd, 1) * rotMat;
							angle = atan2(vectorEnd.z * vectorStart.x - vectorEnd.x * vectorStart.z, vectorEnd.x * vectorStart.x + vectorEnd.z * vectorStart.z);
						}
						else if (gadget.selectedAxis == Gadget::Axis::Y) {
							vectorEnd = glm::vec4(vectorEnd, 1) * rotMat;
							angle = atan2(vectorEnd.y * vectorStart.x - vectorEnd.x * vectorStart.y, vectorEnd.x * vectorStart.x + vectorEnd.y * vectorStart.y);
						}
						else if (gadget.selectedAxis == Gadget::Axis::Z) {
							angle = atan2(vectorEnd.z * vectorStart.x - vectorEnd.x * vectorStart.z, vectorEnd.x * vectorStart.x + vectorEnd.z * vectorStart.z);
						}
						else {
							mouseOffset -= mouseDragStart;
							angle = atan2(vectorEnd.z * vectorStart.x - vectorEnd.x * vectorStart.z, vectorEnd.x * vectorStart.x + vectorEnd.z * vectorStart.z);
						}

						scale = glm::length(vectorEnd) / glm::length(vectorStart);
						angle += glm::pi<float>() / 2;

						if (angle < 0 || angle > glm::pi<float>())
							scale *= -1;
					}
					else {
						mouseOffset -= mouseDragStart;

						if ((gadget.selectedAxis & Gadget::X) == 0)
							mouseOffset.x = 0;
						if ((gadget.selectedAxis & Gadget::Y) == 0)
							mouseOffset.y = 0;
						if ((gadget.selectedAxis & Gadget::Z) == 0)
							mouseOffset.z = 0;
					}

					bool snap = snapToGrid;
					if (ImGui::GetIO().KeyShift)
						snap = !snap;
					
					if (gadget.mode == Gadget::Mode::Rotate || (gadget.mode == Gadget::Mode::Scale && gadget.selectedAxis != Gadget::Axis::XYZ)) {
						simpleShader->setUniform(SimpleShader::Uniforms::shadeType, 1);
						glLineWidth(2);

						if (snap && gridSize >= 1.0f && gadget.mode == Gadget::Mode::Rotate) {
							glm::mat4 gridRotMat = rotMat;
							
							if (gadget.selectedAxis == Gadget::Axis::X) {
								gridRotMat = glm::rotate(gridRotMat, glm::radians(90.0f), glm::vec3(0, 1, 0));
							}
							else if (gadget.selectedAxis == Gadget::Axis::Y) {
								gridRotMat = glm::rotate(gridRotMat, glm::radians(90.0f), glm::vec3(1, 0, 0));
							}
							else {
								gridRotMat = glm::mat4(1);
							}
							
							glm::mat4 modelMatrix = glm::translate(glm::mat4(1), centerGadget);
							
							simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, modelMatrix * gridRotMat);
							simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 1, 1));
							rotateGridVbo->bind();
							glEnableVertexAttribArray(0);
							glEnableVertexAttribArray(1);
							glDisableVertexAttribArray(2);
							glDisableVertexAttribArray(3);
							glDisableVertexAttribArray(4); //TODO: vao
							glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(0 * sizeof(float)));
							glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(3 * sizeof(float)));
							glDrawArrays(GL_LINES, 0, (int)rotateGridVbo->size());
							rotateGridVbo->unBind();
						}

						simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
						std::vector<VertexP3T2N3> verts;
						verts.push_back(VertexP3T2N3(centerGadget, glm::vec2(0), glm::vec3(1.0f)));
						verts.push_back(VertexP3T2N3(mouseOffset, glm::vec2(0), glm::vec3(1.0f)));
						verts.push_back(VertexP3T2N3(centerGadget, glm::vec2(0), glm::vec3(1.0f)));
						verts.push_back(VertexP3T2N3(mouseDragStart, glm::vec2(0), glm::vec3(1.0f)));
						glEnableVertexAttribArray(0);
						glEnableVertexAttribArray(1);
						glEnableVertexAttribArray(2);
						glDisableVertexAttribArray(3);
						glDisableVertexAttribArray(4); //TODO: vao
						glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
						glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
						glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);

						if (gadget.selectedAxis == Gadget::Axis::X) {
							simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0.5f, 0.12f, 0.12f, 1));
						}
						else if (gadget.selectedAxis == Gadget::Axis::Y) {
							simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0.12f, 0.5f, 0.12f, 1));
						}
						else if (gadget.selectedAxis == Gadget::Axis::Z) {
							simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0.12f, 0.12f, 0.5f, 1));
						}
						glDrawArrays(GL_LINES, 0, (int)verts.size());
						simpleShader->setUniform(SimpleShader::Uniforms::shadeType, 0);
					}

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
								n.first->getComponent<RswObject>()->position[gadget.selectedAxisIndex()] = glm::round((n.first->getComponent<RswObject>()->position[gadget.selectedAxisIndex()] - gridOffset) / (float)gridSize) * (float)gridSize + gridOffset;
						}
						if (gadget.mode == Gadget::Mode::Scale)
						{
							if (gadget.selectedAxis == Gadget::Axis::XYZ)
							{
								n.first->getComponent<RswObject>()->scale = n.second.scale * (1 + pos * glm::length(0.01f * mouseOffset));
								if (snap)
									n.first->getComponent<RswObject>()->scale = glm::round(n.first->getComponent<RswObject>()->scale / (float)gridSize) * (float)gridSize;
							}
							else
							{
								n.first->getComponent<RswObject>()->scale[gadget.selectedAxisIndex()] = n.second.scale[gadget.selectedAxisIndex()] * scale;
								if (snap)
									n.first->getComponent<RswObject>()->scale[gadget.selectedAxisIndex()] = glm::round(n.first->getComponent<RswObject>()->scale[gadget.selectedAxisIndex()] / (float)gridSize) * (float)gridSize;
							}
							if (pivotPoint == PivotPoint::GroupCenter)
							{
								if (gadget.selectedAxis == Gadget::Axis::XYZ) {
									float scale = 1 + pos * glm::length(0.01f * mouseOffset);
									if (snap)
										scale = glm::round(scale / (float)gridSize) * (float)gridSize;
									n.first->getComponent<RswObject>()->position = groupCenter + (n.second.pos - groupCenter) * scale;
								}
								else {
									int axis = gadget.selectedAxisIndex();
									n.first->getComponent<RswObject>()->position[axis] = groupCenter[axis] + (n.second.pos[axis] - groupCenter[axis]) * scale;
								}
							}
						}
						if (gadget.mode == Gadget::Mode::Rotate)
						{
							if (snap && pivotPoint == PivotPoint::GroupCenter && map->selectedNodes.size() > 1) {
								angle = glm::radians(glm::round((glm::degrees(angle) - gridOffset) / (float)gridSize) * (float)gridSize + gridOffset);
							}
							
							if (!lockedGizmo) {
								n.first->getComponent<RswObject>()->rotation[gadget.selectedAxisIndex()] = n.second.rot[gadget.selectedAxisIndex()] + glm::degrees(angle);
							}
							else {
								// Tokei: for my sanity, I'm reversing the rotation axis
								glm::mat4 m = glm::mat4(1);
								m = glm::rotate(m, -glm::radians(n.second.rot.z), glm::vec3(0, 0, -1));
								m = glm::rotate(m, glm::radians(n.second.rot.x), glm::vec3(-1, 0, 0));
								m = glm::rotate(m, glm::radians(n.second.rot.y), glm::vec3(0, -1, 0));

								glm::mat4 newRot = glm::mat4(1);

								if (snap && !(pivotPoint == PivotPoint::GroupCenter && map->selectedNodes.size() > 1)) {
									angle = glm::radians(glm::round((glm::degrees(angle) - gridOffset) / (float)gridSize) * (float)gridSize + gridOffset);
								}

								if (gadget.selectedAxis == Gadget::Axis::X) {
									newRot = glm::rotate(newRot, angle, glm::vec3(-1, 0, 0));
								}
								else if (gadget.selectedAxis == Gadget::Axis::Y) {
									newRot = glm::rotate(newRot, angle, glm::vec3(0, -1, 0));
								}
								else {
									newRot = glm::rotate(newRot, -angle, glm::vec3(0, 0, -1));
								}

								m = newRot * m;

								glm::vec3 angles;

								if (m[1][2] < 1) {
									if (m[1][2] > -1) {
										angles = glm::vec3(asin(-m[1][2]), atan2(m[0][2], m[2][2]), -atan2(m[1][0], m[1][1]));
									}
									else {
										angles = glm::vec3(glm::pi<float>() / 2.0f, -atan2(-m[0][1], m[0][0]), 0);
									}
								}
								else {
									angles = glm::vec3(-glm::pi<float>() / 2.0f, atan2(-m[0][1], m[0][0]), 0);
								}

								angles = glm::degrees(angles);

								// Cleanup angles
								if (glm::abs(angles.x) < 0.001f)
									angles.x = 0;
								if (glm::abs(angles.y) < 0.001f)
									angles.y = 0;
								if (glm::abs(angles.z) < 0.001f)
									angles.z = 0;

								n.first->getComponent<RswObject>()->rotation = angles;
							}

							if (!lockedGizmo) {
								if (snap && !(pivotPoint == PivotPoint::GroupCenter && map->selectedNodes.size() > 1)) {
									n.first->getComponent<RswObject>()->rotation[gadget.selectedAxisIndex()] = glm::round((n.first->getComponent<RswObject>()->rotation[gadget.selectedAxisIndex()] - gridOffset) / (float)gridSize) * (float)gridSize + gridOffset;
								}
							}
							
							if (pivotPoint == PivotPoint::GroupCenter)
							{
								float originalAngle = 0;
								float dist = 0;

								if (gadget.selectedAxis == Gadget::Axis::X) {
									originalAngle = atan2(n.second.pos.z - groupCenter.z, n.second.pos.y - groupCenter.y);
									dist = glm::length(glm::vec2(n.second.pos.z - groupCenter.z, n.second.pos.y - groupCenter.y));
									originalAngle += angle;
								}
								else if (gadget.selectedAxis == Gadget::Axis::Y) {
									originalAngle = atan2(n.second.pos.z - groupCenter.z, n.second.pos.x - groupCenter.x);
									dist = glm::length(glm::vec2(n.second.pos.z - groupCenter.z, n.second.pos.x - groupCenter.x));
									originalAngle -= angle;
								}
								else if (gadget.selectedAxis == Gadget::Axis::Z) {
									originalAngle = atan2(n.second.pos.y - groupCenter.y, n.second.pos.x - groupCenter.x);
									dist = glm::length(glm::vec2(n.second.pos.y - groupCenter.y, n.second.pos.x - groupCenter.x));
									originalAngle += angle;
								}

								if (gadget.selectedAxis == Gadget::Axis::X) {
									n.first->getComponent<RswObject>()->position.y = groupCenter.y + dist * glm::cos(originalAngle);
									n.first->getComponent<RswObject>()->position.z = groupCenter.z + dist * glm::sin(originalAngle);
								}
								else if (gadget.selectedAxis == Gadget::Axis::Y) {
									n.first->getComponent<RswObject>()->position.x = groupCenter.x + dist * glm::cos(originalAngle);
									n.first->getComponent<RswObject>()->position.z = groupCenter.z + dist * glm::sin(originalAngle);
								}
								else if (gadget.selectedAxis == Gadget::Axis::Z) {
									n.first->getComponent<RswObject>()->position.x = groupCenter.x + dist * glm::cos(originalAngle);
									n.first->getComponent<RswObject>()->position.y = groupCenter.y + dist * glm::sin(originalAngle);
								}
							}
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
		static bool justPressed = false;
		static glm::vec3 originalPosition;
		static float originalHeightFromGround;
		if (ImGui::IsMouseClicked(0))
		{
			auto collisions = map->rootNode->getCollisions(mouseRay);
			std::erase_if(collisions, [&](const std::pair<Node*, std::vector<glm::vec3>>& pn)
			{
				if (!viewModels && pn.first->getComponent<RswModel>())
					return true;
				if (!viewEffects && pn.first->getComponent<RswEffect>())
					return true;
				if (!viewSounds && pn.first->getComponent<RswSound>())
					return true;
				if (!viewLights && pn.first->getComponent<RswLight>())
					return true;
				return false;
			});
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

				if (map->selectedNodes.size() == 1 && map->selectedNodes[0] == collisions[closest].first)
				{
					if (map->selectedNodes[0]->getComponent<RswObject>())
					{
						originalPosition = map->selectedNodes[0]->getComponent<RswObject>()->position;
						justPressed = true;

						if (map->selectedNodes[0]->getComponent<RswLight>()) {
							glm::vec3 wPosition(gnd->width * 5.0f + originalPosition.x, -originalPosition.y, gnd->height * 5.0f - originalPosition.z + 10.0f);
							
							math::Ray ray(wPosition, glm::vec3(0, -1, 0));
							auto rayCast = gnd->rayCast(ray, viewEmptyTiles);

							if (rayCast != glm::vec3(std::numeric_limits<float>().max())) {
								originalHeightFromGround = rayCast.y - wPosition.y;
							}
							else {
								originalHeightFromGround = 0;
							}
						}
					}
				}

				map->doAction(new SelectAction(map, collisions[closest].first, ImGui::GetIO().KeyShift, std::find(map->selectedNodes.begin(), map->selectedNodes.end(), collisions[closest].first) != map->selectedNodes.end() && ImGui::GetIO().KeyShift), browEdit);
			}
			objectSelectLasso.clear();
		}
		if (ImGui::IsMouseDown(0))
		{
			auto rayCast = gnd->rayCast(mouseRay, viewEmptyTiles);
			if (justPressed && rayCast != glm::vec3(std::numeric_limits<float>().max()))
			{
				if (map->selectedNodes.size() == 1 && ImGui::IsMouseDragging(0))
				{
					auto rswObject = map->selectedNodes[0]->getComponent<RswObject>(); 
					if (rswObject)
					{
						if (map->selectedNodes.size() == 1 && map->selectedNodes[0]->getComponent<RswLight>()) {
							rswObject->position = glm::vec3(rayCast.x - 5 * gnd->width, -rayCast.y + originalHeightFromGround, -(rayCast.z + (-10 - 5 * gnd->height)));
						}
						else {
							rswObject->position = glm::vec3(rayCast.x - 5 * gnd->width, -rayCast.y, -(rayCast.z + (-10 - 5 * gnd->height)));

							auto rsm = map->selectedNodes[0]->getComponent<Rsm>();
							if (rsm && rsm->version >= 0x202) {
								rswObject->position.y += rsm->bbmin.y;
							}
						}

						bool snap = snapToGrid;
						if (ImGui::GetIO().KeyShift)
							snap = !snap;
						if (snap)
						{
							if (!gridLocal)
							{
								rswObject->position.x = glm::round((rswObject->position.x - gridOffset) / gridSize) * gridSize + gridOffset;
								rswObject->position.z = glm::round((rswObject->position.z - gridOffset) / gridSize) * gridSize + gridOffset;
							}
							else
							{
								glm::vec3 diff = rswObject->position - originalPosition;
								diff.x = glm::round(diff.x / gridSize) * gridSize;
								diff.z = glm::round(diff.z / gridSize) * gridSize;
								rswObject->position = originalPosition + diff;
							}
						}
					}
					
					auto rsmRenderer = map->selectedNodes[0]->getComponent<RsmRenderer>();
					if (rsmRenderer)
						rsmRenderer->matrixCached = false;
				}
			}
			else
				if (rayCast != glm::vec3(std::numeric_limits<float>().max()) && (objectSelectLasso.size() == 0 || rayCast != objectSelectLasso.back()))
					objectSelectLasso.push_back(rayCast);
		}
		if (ImGui::IsMouseReleased(0))
		{
			if (justPressed && map->selectedNodes.size() > 0 && map->selectedNodes[0]->getComponent<RswObject>())
			{
				if(map->selectedNodes[0]->getComponent<RswObject>()->position != originalPosition)
					map->doAction(new ObjectChangeAction(map->selectedNodes[0], &map->selectedNodes[0]->getComponent<RswObject>()->position, originalPosition, "Moved"), browEdit);
				justPressed = false;
			}
			else if (objectSelectLasso.size() > 5)
			{
				math::Polygon polygon;
				for(auto& p : objectSelectLasso)
					polygon.push_back(glm::vec2(p.x, p.z));

				bool modified = false;
				std::vector<Node*> newSelectedNodes;

				if (ImGui::GetIO().KeyShift)
					newSelectedNodes = map->selectedNodes;

				auto ga = new GroupAction();
				map->rootNode->traverse([&](Node* n)
				{
					auto rswObject = n->getComponent<RswObject>();
					if (!rswObject)
						return;

					glm::vec2 pos(5 * gnd->width + rswObject->position.x, -(-10 - 5 * gnd->height + rswObject->position.z));
					if (polygon.contains(pos))
					{
						if (ImGui::GetIO().KeyShift) {
							if (std::find(map->selectedNodes.begin(), map->selectedNodes.end(), n) != map->selectedNodes.end()) {
								newSelectedNodes.erase(std::remove_if(newSelectedNodes.begin(), newSelectedNodes.end(), [n](Node* n2) { return n2 == n; }));
							}
							else {
								newSelectedNodes.push_back(n);
							}
						}
						else {
							newSelectedNodes.push_back(n);
						}
						modified = true;
					}
				});

				if (modified) {
					auto action = new SelectAction(map, newSelectedNodes, false, false);
					map->doAction(action, browEdit);
				}
			}
			objectSelectLasso.clear();
		}
	}
	fbo->unbind();
}


 

void MapView::rebuildObjectModeGrid()
{
	std::vector<VertexP3T2> verts;
	if (glm::abs(gridSizeTranslate) > 0)
	{
		for (float i = -10 * gridSizeTranslate; i <= 10 * gridSizeTranslate; i += gridSizeTranslate)
		{
			verts.push_back(VertexP3T2(glm::vec3(-10 * gridSizeTranslate, 0, i), glm::vec2(0)));
			verts.push_back(VertexP3T2(glm::vec3(10 * gridSizeTranslate, 0, i), glm::vec2(0)));

			verts.push_back(VertexP3T2(glm::vec3(i, 0, -10 * gridSizeTranslate), glm::vec2(0)));
			verts.push_back(VertexP3T2(glm::vec3(i, 0, 10 * gridSizeTranslate), glm::vec2(0)));
		}
	}
	gridVbo->setData(verts, GL_STATIC_DRAW);
}

void MapView::rebuildObjectRotateModeGrid()
{
	std::vector<VertexP3T2> verts;
	if (glm::abs(gridSizeRotate) >= 1.0f)
	{
		float size = 25 * gadget.scale;
		float gridAngle = 0;

		glm::vec3 v1;
		glm::vec3 v2;

		while (gridAngle <= 360.0f) {
			v1 = glm::vec3(glm::cos(glm::radians(-gridAngle - gridOffsetRotate)), glm::sin(glm::radians(-gridAngle - gridOffsetRotate)), 0);
			v2 = v1 * (size + 6.0f);
			v1 = v1 * (size + 2.0f);

			v1 = glm::vec4(v1, 1);
			v2 = glm::vec4(v2, 1);

			verts.push_back(VertexP3T2(v1, glm::vec2(0)));
			verts.push_back(VertexP3T2(v2, glm::vec2(0)));

			gridAngle += gridSizeRotate;
		}
	}
	rotateGridVbo->setData(verts, GL_STATIC_DRAW);
}