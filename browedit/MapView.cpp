#define IMGUI_DEFINE_MATH_OPERATORS
#include <Windows.h>
#include "MapView.h"
#include "Map.h"
#include "BrowEdit.h"
#include "Node.h"
#include "Icons.h"
#include "Gadget.h"
#include "components/Rsw.h"
#include "components/Gnd.h"
#include "components/GndRenderer.h"
#include "components/RsmRenderer.h"
#include "components/GatRenderer.h"
#include "components/WaterRenderer.h"
#include "components/BillboardRenderer.h"

#include <browedit/HotkeyRegistry.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Shader.h>
#include <browedit/gl/Vertex.h>
#include <browedit/gl/Texture.h>
#include <browedit/math/Ray.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/shaders/SimpleShader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <GLFW/glfw3.h>

#include <iostream>

std::vector<std::vector<glm::vec3>> debugPoints;
std::mutex debugPointMutex;

MapView::MapView(Map* map, const std::string &viewName) : map(map), viewName(viewName), mouseRay(glm::vec3(0,0,0), glm::vec3(0,0,0))
{
	fbo = new gl::FBO(1920, 1080, true);
	//shader = new TestShader();

	auto gnd = map->rootNode->getComponent<Gnd>();
	if (gnd)
	{
		cameraCenter.x = gnd->width * 5.0f;
		cameraCenter.y = 0;
		cameraCenter.z = gnd->height * 5.0f;
	}
	billboardShader = util::ResourceManager<gl::Shader>::load<BillboardRenderer::BillboardShader>();
	whiteTexture = util::ResourceManager<gl::Texture>::load("data\\texture\\white.png");
	if (!sphereMesh.vbo)
		sphereMesh.init();
	if (!cubeMesh.vbo)
	{
		cubeMesh.init();
		cubeTexture = util::ResourceManager<gl::Texture>::load("data\\model\\cube2.png");
	}
	if (!simpleShader)
		simpleShader = util::ResourceManager<gl::Shader>::load<SimpleShader>();

	for(int i = 0; i < 9; i++)
		gadgetHeight[i].mode = Gadget::Mode::TranslateY;

	gridVbo = new gl::VBO<VertexP3T2>();
	textureGridVbo = new gl::VBO<VertexP3T2>();
	rebuildObjectModeGrid();

}

void MapView::toolbar(BrowEdit* browEdit)
{
	browEdit->toolBarToggleButton("ortho", ortho ? ICON_ORTHO : ICON_PERSPECTIVE, &ortho, "Toggle between ortho and perspective camera", browEdit->config.toolbarButtonsViewOptions);
	ImGui::SameLine();

	if (showViewOptions)
	{
		ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos() + ImVec2(0, browEdit->config.toolbarButtonSize));
		if (ImGui::Begin(("ViewOptions##" + viewName).c_str(), 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
		{
			ImGui::Text("Render Settings");
			browEdit->toolBarToggleButton("viewLightMapShadow", viewLightmapShadow ? ICON_SHADOWMAP_ON : ICON_SHADOWMAP_OFF, viewLightmapShadow, "Toggle shadowmap", HotkeyAction::View_ShadowMap, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewLightmapColor", viewLightmapColor ? ICON_COLORMAP_ON : ICON_COLORMAP_OFF, viewLightmapColor, "Toggle colormap", HotkeyAction::View_ColorMap, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewColors", viewColors ? ICON_TILECOLOR_ON : ICON_TILECOLOR_OFF, viewColors, "Toggle tile colors", HotkeyAction::View_TileColors, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewLighting", viewLighting ? ICON_LIGHTING_ON : ICON_LIGHTING_OFF, viewLighting, "Toggle lighting", HotkeyAction::View_Lighting, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewTextures", viewTextures ? ICON_TEXTURE_ON : ICON_TEXTURE_OFF, viewTextures, "Toggle textures", HotkeyAction::View_Textures, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("smoothColors", smoothColors ? ICON_SMOOTH_COLOR_ON : ICON_SMOOTH_COLOR_OFF, smoothColors, "Smooth colormap", HotkeyAction::View_SmoothColormap, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewEmptyTiles", viewEmptyTiles ? ICON_EMPTYTILE_ON : ICON_EMPTYTILE_OFF, viewEmptyTiles, "View empty tiles", HotkeyAction::View_EmptyTiles, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			if(browEdit->editMode == BrowEdit::EditMode::Gat)
				browEdit->toolBarToggleButton("viewGat", viewGatGat ? ICON_EDIT_GAT : ICON_EDIT_GAT, viewGatGat, "View GAT tiles", HotkeyAction::View_GatTiles, browEdit->config.toolbarButtonsGatEdit);
			else
				browEdit->toolBarToggleButton("viewGat", viewGat ? ICON_EDIT_GAT : ICON_EDIT_GAT, viewGat, "View GAT tiles", HotkeyAction::View_GatTiles, browEdit->config.toolbarButtonsGatEdit);

			if (browEdit->editMode == BrowEdit::EditMode::Gat ? viewGatGat : viewGat)
				ImGui::DragFloat("Gat Opacity", &gatOpacity, 0.025f, 0.0f, 1.0f);


			ImGui::Text("Object render settings");
			browEdit->toolBarToggleButton("viewModels", viewModels ? ICON_VIEW_MODEL_ON : ICON_VIEW_MODEL_OFF, &viewModels, "View Models", browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewEffects", viewEffects ? ICON_VIEW_EFFECT_ON : ICON_VIEW_EFFECT_OFF, &viewEffects, "View Effects", browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewSounds", viewSounds ? ICON_VIEW_SOUND_ON : ICON_VIEW_SOUND_OFF, &viewSounds, "View Sounds", browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewLights", viewLights ? ICON_VIEW_LIGHT_ON : ICON_VIEW_LIGHT_OFF, &viewLights, "View Lights", browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewWater", viewWater ? ICON_VIEW_WATER_ON : ICON_VIEW_WATER_OFF, &viewWater, "View Water", browEdit->config.toolbarButtonsViewOptions);

			if (browEdit->editMode == BrowEdit::EditMode::Object)
			{
				ImGui::Text("Light render settings");
				browEdit->toolBarToggleButton("showAllLights", showAllLights ? ICON_ALL_LIGHTS_ON : ICON_ALL_LIGHTS_OFF, &showAllLights, "Show all lights", browEdit->config.toolbarButtonsViewOptions);
				ImGui::SameLine();
				browEdit->toolBarToggleButton("showLightSphere", showLightSphere ? ICON_LIGHTSPHERE_ON : ICON_LIGHTSPHERE_OFF, &showLightSphere, "Show light sphere", browEdit->config.toolbarButtonsViewOptions);
				ImGui::SameLine();
			}
			ImGui::End();
		}
	}
	browEdit->toolBarToggleButton("showViewOptions", ICON_VIEWOPTIONS, &showViewOptions, "View Options", browEdit->config.toolbarButtonsViewOptions);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::DragInt("##quadTreeMaxLevel", &quadTreeMaxLevel, 1, 0, 6);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Change quadtree detail level");
	ImGui::SameLine();


	bool snapping = snapToGrid;
	if (ImGui::GetIO().KeyShift)
		snapping = !snapping;
	bool ret = browEdit->toolBarToggleButton("snapToGrid", snapping ? ICON_GRID_SNAP_ON : ICON_GRID_SNAP_OFF, snapping, "Snap to grid");
	if (!ImGui::GetIO().KeyShift && ret)
		snapToGrid = !snapToGrid;
	if (snapping || snapToGrid)
	{
		ImGui::SameLine();
		ImGui::SetNextItemWidth(50);
		if (ImGui::DragFloat("##gridSize", (gadget.mode == Gadget::Mode::Translate || browEdit->editMode == BrowEdit::EditMode::Height) ? &gridSizeTranslate : &gridSizeRotate, 1.0f, 0.1f, 100.0f, "%.2f"))
			rebuildObjectModeGrid();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Grid size. Doubleclick or ctrl+click to type a number");
		if (ImGui::BeginPopupContextItem("GridSize"))
		{
			auto gridSizes = &browEdit->config.translateGridSizes;
			if(gadget.mode == Gadget::Mode::Rotate && browEdit->editMode != BrowEdit::EditMode::Height)
				gridSizes = &browEdit->config.rotateGridSizes;
			for (auto f : *gridSizes)
			{
				if (ImGui::Button(std::to_string(f).c_str()))
				{
					if (gadget.mode == Gadget::Mode::Translate)
						gridSizeTranslate = f;
					else
						gridSizeRotate = f;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		ImGui::Checkbox("##gridLocal", &gridLocal);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Local or Global grid. Either makes the movement rounded off, or the final position");
		if (!gridLocal)
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(50);
			if (gadget.mode == Gadget::Mode::Translate)
				ImGui::DragFloat("##gridOffset", &gridOffsetTranslate, 1.0f, 0, gridSizeTranslate, "%.2f");
			else
				ImGui::DragFloat("##gridOffset", &gridOffsetRotate, 1.0f, 0, gridSizeRotate, "%.2f");
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Grid Offset");
		}
		if (browEdit->editMode == BrowEdit::EditMode::Texture)
		{
			ImGui::SameLine(); //TODO: make an icon for these
			if (browEdit->toolBarToggleButton("smartGrid", textureGridSmart ? ICON_GRID_SNAP_ON : ICON_GRID_SNAP_OFF, &textureGridSmart, "Smart Grid"))
				textureGridDirty = true;
		}

	}


	if (browEdit->editMode == BrowEdit::EditMode::Object)
	{
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("translate", ICON_MOVE, gadget.mode == Gadget::Mode::Translate, "Move", browEdit->config.toolbarButtonsObjectEdit))
			gadget.mode = Gadget::Mode::Translate;
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("rotate", ICON_ROTATE, gadget.mode == Gadget::Mode::Rotate, "Rotate", browEdit->config.toolbarButtonsObjectEdit))
			gadget.mode = Gadget::Mode::Rotate;
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("scale", ICON_SCALE, gadget.mode == Gadget::Mode::Scale, "Scale", browEdit->config.toolbarButtonsObjectEdit))
			gadget.mode = Gadget::Mode::Scale;


		if (gadget.mode == Gadget::Mode::Rotate || gadget.mode == Gadget::Mode::Scale)
		{
			ImGui::SameLine();
			if (browEdit->toolBarToggleButton("localglobal", pivotPoint == MapView::PivotPoint::Local ? ICON_PIVOT_ROT_LOCAL : ICON_PIVOT_ROT_GLOBAL, false, "Changes the pivot point for rotations", browEdit->config.toolbarButtonsObjectEdit))
			{
				if (pivotPoint == MapView::PivotPoint::Local)
					pivotPoint = MapView::PivotPoint::GroupCenter;
				else
					pivotPoint = MapView::PivotPoint::Local;
			}

		}

	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::SliderFloat("##gadgetOpacity", &gadgetOpacity, 0, 1);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Gadget Opacity");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(50);
	ImGui::SliderFloat("##gadgetScale", &gadgetScale, 0, 2);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Gadget scale");
	ImGui::SetNextItemWidth(50);
	ImGui::SameLine();
	if (ImGui::SliderFloat("##gadgetThickness", &gadgetThickness, 0, 5))
	{
		Gadget::setThickness(gadgetThickness);
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Gadget thickness");
	Gadget::opacity = gadgetOpacity;
	Gadget::scale = gadgetScale;
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
	nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, -cameraCenter);

	//TODO: fix this, don't want to individually set settings
	map->rootNode->getComponent<GndRenderer>()->viewLightmapShadow = viewLightmapShadow;
	map->rootNode->getComponent<GndRenderer>()->viewLightmapColor = viewLightmapColor;
	map->rootNode->getComponent<GndRenderer>()->viewColors = viewColors;
	map->rootNode->getComponent<GndRenderer>()->viewLighting = viewLighting;
	map->rootNode->getComponent<GndRenderer>()->viewTextures = viewTextures;
	if (map->rootNode->getComponent<GndRenderer>()->viewEmptyTiles != viewEmptyTiles)
	{
		map->rootNode->getComponent<GndRenderer>()->viewEmptyTiles = viewEmptyTiles;
		auto gnd = map->rootNode->getComponent<Gnd>();
		for (int x = 0; x < gnd->width; x++)
			for (int y = 0; y < gnd->height - 15; y++)
				map->rootNode->getComponent<GndRenderer>()->setChunkDirty(x,y);
	}
	if (map->rootNode->getComponent<GndRenderer>()->smoothColors != smoothColors)
	{
		map->rootNode->getComponent<GndRenderer>()->smoothColors = smoothColors;
		map->rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
	}

	RsmRenderer::RsmRenderContext::getInstance()->viewLighting = viewLighting;
	RsmRenderer::RsmRenderContext::getInstance()->viewTextures = viewTextures;

	map->rootNode->getComponent<GatRenderer>()->cameraDistance = cameraDistance;
	map->rootNode->getComponent<GatRenderer>()->enabled = browEdit->editMode == BrowEdit::EditMode::Gat ? viewGatGat : viewGat;
	map->rootNode->getComponent<GatRenderer>()->opacity = gatOpacity;


	//TODO: this does not seem so efficient
	for (auto rc : nodeRenderContext.renderers[map->rootNode])
	{
		for (auto r : rc.second)
		{
			if (r->node->getComponent<RswModel>())
				r->enabled = viewModels;
			if (r->node->getComponent<RswEffect>())
				r->enabled = viewEffects;
			if (r->node->getComponent<RswSound>())
				r->enabled = viewSounds;
			if (r->node->getComponent<RswLight>())
				r->enabled = viewLights;
		}
	}
	map->rootNode->getComponent<WaterRenderer>()->enabled = viewWater;

	NodeRenderer::render(map->rootNode, nodeRenderContext);

	// position and draw the new nodes
	if (browEdit->newNodes.size() > 0)
	{
		auto gnd = map->rootNode->getComponent<Gnd>();
		auto rayCast = gnd->rayCast(mouseRay);
		for (auto newNode : browEdit->newNodes)
		{
			auto rswObject = newNode.first->getComponent<RswObject>();

			bool snap = snapToGrid;
			if (ImGui::GetIO().KeyShift)
				snap = !snap;
			if (snap)
			{
				rayCast.x = glm::round((rayCast.x - gridOffsetTranslate) / (float)gridSizeTranslate) * (float)gridSizeTranslate + gridOffsetTranslate;
				rayCast.z = glm::round((rayCast.z - gridOffsetTranslate) / (float)gridSizeTranslate) * (float)gridSizeTranslate + gridOffsetTranslate;
			}

			rswObject->position = glm::vec3(rayCast.x - 5 * gnd->width, -rayCast.y, -(rayCast.z + (-10 - 5 * gnd->height))) + newNode.second;
			if (browEdit->newNodeHeight)
			{
				rswObject->position.y = newNode.second.y + browEdit->newNodesCenter.y;
			}

			if (newNode.first->getComponent<RsmRenderer>())
			{
				glm::mat4 matrix = glm::mat4(1.0f);
				matrix = glm::translate(matrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -(- 10 - 5 * gnd->height + rswObject->position.z)));
				matrix = glm::rotate(matrix, -glm::radians(rswObject->rotation.z), glm::vec3(0, 0, 1));
				matrix = glm::rotate(matrix, -glm::radians(rswObject->rotation.x), glm::vec3(1, 0, 0));
				matrix = glm::rotate(matrix, glm::radians(rswObject->rotation.y), glm::vec3(0, 1, 0));
				matrix = glm::scale(matrix, glm::vec3(1, -1, 1));
				matrix = glm::scale(matrix, rswObject->scale);
				matrix = glm::scale(matrix, glm::vec3(1, 1, -1));
				newNode.first->getComponent<RsmRenderer>()->matrixCache = matrix;
			}
			if (newNode.first->getComponent<BillboardRenderer>())
				newNode.first->getComponent<BillboardRenderer>()->gnd = gnd;
			NodeRenderer::render(newNode.first, nodeRenderContext);
		}
	}

	glUseProgram(0);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(nodeRenderContext.projectionMatrix));
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(nodeRenderContext.viewMatrix));
	glColor3f(1, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_2D);


	glLineWidth(1.0f);
	//draw quadtree
	glPushMatrix();
	glScalef(1, -1, -1);
	auto gnd = map->rootNode->getComponent<Gnd>();
	glTranslatef(gnd->width * 5.0f, 0.0f, -gnd->height * 5.0f - 10);
	map->rootNode->getComponent<Rsw>()->quadtree->draw(quadTreeMaxLevel);
	glPopMatrix();


	//draw debug points
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

	fbo->unbind();
}


//update just fixes the camera
void MapView::update(BrowEdit* browEdit, const ImVec2 &size, float deltaTime)
{
	if (fbo->getWidth() != (int)size.x || fbo->getHeight() != (int)size.y)
		fbo->resize((int)size.x, (int)size.y);
	mouseState.position = glm::vec2(ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x, ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y);
	mouseState.buttons = (ImGui::IsMouseDown(0) ? 0x01 : 0x00) | 
		(ImGui::IsMouseDown(1) ? 0x02 : 0x00) |
		(ImGui::IsMouseDown(2) ? 0x04 : 0x00);

	if (cameraAnimating)
	{
		float dX = cameraTargetRotX - cameraRotX;
		float dY = cameraTargetRotY - cameraRotY;
		if (dX > 180)
			dX -= 360;
		if (dX < -180)
			dX += 360;
		if (dY > 180)
			dY -= 360;
		if (dY < -180)
			dY += 360;

		cameraRotX += 180 * deltaTime * glm::sign(dX);
		cameraRotY += 180 * deltaTime * glm::sign(dY);


		
		if(glm::distance(cameraCenter, cameraTargetPos) < 2000.0f * deltaTime)
			cameraCenter = cameraTargetPos;
		else
			cameraCenter -= glm::normalize(cameraCenter - cameraTargetPos) * 2000.0f * deltaTime;

		if (glm::abs(dX) < 180 * deltaTime && glm::abs(dY) < 180 * deltaTime)
		{
			cameraRotX = cameraTargetRotX;
			cameraRotY = cameraTargetRotY;
		}

		if (glm::abs(dX) < 180 * deltaTime && glm::abs(dY) < 180 * deltaTime && glm::distance(cameraCenter, cameraTargetPos) < 2000.0f * deltaTime)
			cameraAnimating = false;
	}

	if (hovered)
	{
		if ((mouseState.buttons&6) != 0)
		{
			if (ImGui::GetIO().KeyShift)
			{
				cameraRotX += (mouseState.position.y - prevMouseState.position.y) * 0.25f * browEdit->config.cameraMouseSpeed;
				cameraRotY += (mouseState.position.x - prevMouseState.position.x) * 0.25f * browEdit->config.cameraMouseSpeed;
				cameraRotX = glm::clamp(cameraRotX, 0.0f, 90.0f);
			}
			else if (ImGui::GetIO().KeyCtrl)
			{
				cameraCenter.y += (mouseState.position.y - prevMouseState.position.y);
			}
			else
			{
				cameraCenter -= glm::vec3(glm::vec4(
					(mouseState.position.x - prevMouseState.position.x) * browEdit->config.cameraMouseSpeed,
					0,
					(mouseState.position.y - prevMouseState.position.y) * browEdit->config.cameraMouseSpeed, 0)
					* glm::rotate(glm::mat4(1.0f), glm::radians(cameraRotY), glm::vec3(0, 1, 0)));

				auto rayCast = map->rootNode->getComponent<Gnd>()->rayCast(math::Ray(cameraCenter + glm::vec3(0,9999,0), glm::vec3(0,-1,0)), viewEmptyTiles);
				if (rayCast != glm::vec3(0, 0, 0))
					cameraCenter.y = 0.95f * cameraCenter.y + 0.05f * rayCast.y;
			}
		}
		cameraDistance *= (1 - (ImGui::GetIO().MouseWheel * 0.1f));
		cameraDistance = glm::clamp(0.0f, 4000.0f, cameraDistance);
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



bool MapView::drawCameraWidget()
{
	fbo->bind();
	cubeTexture->bind();
	simpleShader->use();

	glm::mat4 projectionMatrix = glm::ortho(-fbo->getWidth() + 50.0f, 50.0f, -fbo->getHeight() + 50.0f, 50.0f, -1000.0f, 1000.0f);
	glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
	viewMatrix = glm::rotate(viewMatrix, glm::radians(cameraRotX), glm::vec3(1, 0, 0));
	viewMatrix = glm::rotate(viewMatrix, glm::radians(cameraRotY), glm::vec3(0, 1, 0));
	viewMatrix = glm::scale(viewMatrix, glm::vec3(40, 40, 40));

	simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, projectionMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, viewMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));

	simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
	simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
	simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1,1,1,1));
	glDisable(GL_BLEND);

	cubeMesh.draw();
	glUseProgram(0);
	fbo->unbind();



	{
		int viewPort[4]{ 0, 0, fbo->getWidth(), fbo->getHeight() };
		glm::vec2 mousePosScreenSpace(mouseState.position);
		mousePosScreenSpace.y = fbo->getHeight() - mousePosScreenSpace.y;
		glm::vec3 retNear = glm::unProject(glm::vec3(mousePosScreenSpace, 0.0f), viewMatrix, projectionMatrix, glm::vec4(viewPort[0], viewPort[1], viewPort[2], viewPort[3]));
		glm::vec3 retFar = glm::unProject(glm::vec3(mousePosScreenSpace, 1.0f), viewMatrix, projectionMatrix, glm::vec4(viewPort[0], viewPort[1], viewPort[2], viewPort[3]));
		
		math::Ray ray(retNear, glm::normalize(retFar-retNear));
		if (cubeMesh.collision(ray, glm::mat4(1.0f)))
		{
			if (ImGui::IsMouseClicked(0))
			{
				glm::vec3 point = cubeMesh.getCollision(ray, glm::mat4(1.0f));
				//std::cout << point.x << "\t" << point.y << "\t" << point.z << std::endl;
				if (glm::abs(point.y - 1) < 0.001)
				{
					cameraTargetRotY = 0;
					cameraTargetRotX = 90;
					cameraTargetPos = cameraCenter;
					cameraAnimating = true;
				}
				else if (glm::abs(point.z) - 1 < 0.001 || glm::abs(point.x) - 1 < 0.001) //side
				{
					float angle = std::atan2(glm::round(point.z), glm::round(point.x));
					cameraTargetRotY = glm::degrees(angle)-90.0f;
					cameraTargetRotX = 0;
					if (glm::abs(point.y-1) < 0.25f)
						cameraTargetRotX = 45;
					cameraTargetPos = cameraCenter;
					cameraAnimating = true;
				}

			}
			return true;
		}
	}
	return false;
}


void MapView::focusSelection()
{
	cameraAnimating = true;
	cameraTargetRotX = cameraRotX;
	cameraTargetRotY = cameraRotY;
	auto center = map->getSelectionCenter();
	auto gnd = map->rootNode->getComponent<Gnd>();
	cameraTargetPos = glm::vec3(5*gnd->width + center.x, -center.y, 5*gnd->height - center.z);
}


namespace icosphere
{
	struct Triangle
	{
		int vertex[3];
	};

	using TriangleList = std::vector<Triangle>;
	using VertexList = std::vector<glm::vec3>;

	const float X = .525731112119133606f;
	const float Z = .850650808352039932f;
	const float N = 0.f;

	static const VertexList beginVertices =
	{
		{-X,N,Z}, {X,N,Z}, {-X,N,-Z}, {X,N,-Z},
		{N,Z,X}, {N,Z,-X}, {N,-Z,X}, {N,-Z,-X},
		{Z,X,N}, {-Z,X, N}, {Z,-X,N}, {-Z,-X, N}
	};

	static const TriangleList beginTriangles =
	{
		{0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
		{8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
		{7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
		{6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
	};


	using Lookup = std::map<std::pair<int, int>, int>;

	int vertex_for_edge(Lookup& lookup,
		VertexList& vertices, int first, int second)
	{
		Lookup::key_type key(first, second);
		if (key.first > key.second)
			std::swap(key.first, key.second);

		auto inserted = lookup.insert({ key, (int)vertices.size() });
		if (inserted.second)
		{
			auto& edge0 = vertices[first];
			auto& edge1 = vertices[second];
			auto point = normalize(edge0 + edge1);
			vertices.push_back(point);
		}

		return inserted.first->second;
	}
	TriangleList subdivide(VertexList& vertices,
		TriangleList triangles)
	{
		Lookup lookup;
		TriangleList result;

		for (auto&& each : triangles)
		{
			std::array<int, 3> mid;
			for (int edge = 0; edge < 3; ++edge)
			{
				mid[edge] = vertex_for_edge(lookup, vertices,
					each.vertex[edge], each.vertex[(edge + 1) % 3]);
			}

			result.push_back({ each.vertex[0], mid[0], mid[2] });
			result.push_back({ each.vertex[1], mid[1], mid[0] });
			result.push_back({ each.vertex[2], mid[2], mid[1] });
			result.push_back({ mid[0], mid[1], mid[2] });
		}

		return result;
	}

	using IndexedMesh = std::pair<VertexList, TriangleList>;
	IndexedMesh make_icosphere(int subdivisions)
	{
		VertexList vertices = beginVertices;
		TriangleList triangles = beginTriangles;

		for (int i = 0; i < subdivisions; ++i)
		{
			triangles = subdivide(vertices, triangles);
		}

		return{ vertices, triangles };
	}
}

void MapView::SphereMesh::buildVertices(std::vector<VertexP3T2N3>& verts)
{
	auto ico = icosphere::make_icosphere(3);
	for (const auto &triangle : ico.second)
		for(int i = 0; i < 3; i++)
			verts.push_back(ico.first[triangle.vertex[i]]);
}

void MapView::drawLight(Node* n)
{
	auto rswObject = n->getComponent<RswObject>();
	auto rswLight = n->getComponent<RswLight>();
	auto gnd = n->root->getComponent<Gnd>();

	glm::mat4 modelMatrix(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1, 1, -1));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));
	if (showLightSphere)
	{
		modelMatrix = glm::scale(modelMatrix, glm::vec3(rswLight->range, rswLight->range, rswLight->range));
		simpleShader->use();
		simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
		simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
		simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, modelMatrix);
		simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(rswLight->color, 0.25f));
		glDepthMask(0);
		glEnable(GL_BLEND);
		sphereMesh.draw();
		glDepthMask(1);
		glUseProgram(0);
	}
	else
	{
		std::vector<VertexP3T2> vertsCircle;
		for (float f = 0; f < 2 * glm::pi<float>(); f += glm::pi<float>() / 50)
			vertsCircle.push_back(VertexP3T2(glm::vec3(rswLight->range * glm::cos(f), rswLight->range * glm::sin(f), 0), glm::vec2(glm::cos(f), glm::sin(f))));
		billboardShader->use();
		billboardShader->setUniform(BillboardRenderer::BillboardShader::Uniforms::cameraMatrix, nodeRenderContext.viewMatrix);
		billboardShader->setUniform(BillboardRenderer::BillboardShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
		billboardShader->setUniform(BillboardRenderer::BillboardShader::Uniforms::modelMatrix, modelMatrix);
		billboardShader->setUniform(BillboardRenderer::BillboardShader::Uniforms::color, glm::vec4(rswLight->color, 1));
		whiteTexture->bind();
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4); //TODO: vao
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), vertsCircle[0].data);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), vertsCircle[0].data + 3);
		glLineWidth(4.0f);
		glDrawArrays(GL_LINE_LOOP, 0, (int)vertsCircle.size());
		glUseProgram(0);
	}
}





void MapView::CubeMesh::buildVertices(std::vector<VertexP3T2N3> &verts)
{//https://dotnetfiddle.net/tPcv8S
	//https://gist.github.com/Borf/940cc411e814a0164a361f34bd13c4ee
	verts.push_back({ glm::vec3(0.66667, -1, -0.66667), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(-0.66667, -1, 0.66667), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(-0.66667, -1, -0.66667), glm::vec2(0.125, 0.4375) });
	verts.push_back({ glm::vec3(-1, 0.66667, 0.66667), glm::vec2(0.304688, 0.304688) });
	verts.push_back({ glm::vec3(-1, -0.66667, -0.66667), glm::vec2(0.070312, 0.070312) });
	verts.push_back({ glm::vec3(-1, -0.66667, 0.66667), glm::vec2(0.304688, 0.070312) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, 1), glm::vec2(0.929688, 0.617188) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, 1), glm::vec2(0.695312, 0.382812) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, 1), glm::vec2(0.929688, 0.382812) });
	verts.push_back({ glm::vec3(-0.66667, 1, -0.66667), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(0.66667, 1, 0.66667), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(0.66667, 1, -0.66667), glm::vec2(0.125, 0.4375) });
	verts.push_back({ glm::vec3(1, 0.66667, -0.66667), glm::vec2(0.617188, 0.304688) });
	verts.push_back({ glm::vec3(1, -0.66667, 0.66667), glm::vec2(0.382812, 0.070312) });
	verts.push_back({ glm::vec3(1, -0.66667, -0.66667), glm::vec2(0.617188, 0.070312) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, -1), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(0.66667, 1, -0.66667), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(1, 0.66667, -0.66667), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(0.66667, -1, -0.66667), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, -1), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(1, -0.66667, -0.66667), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(1, 0.66667, 0.66667), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(0.66667, 1, 0.66667), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, 1), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(1, -0.66667, 0.66667), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, 1), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(0.66667, -1, 0.66667), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, -1), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-1, 0.66667, -0.66667), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, 1, -0.66667), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(-1, -0.66667, -0.66667), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, -1), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, -1, -0.66667), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(-1, 0.66667, 0.66667), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, 1), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, 1, 0.66667), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, -1, 0.66667), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, 1), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(-1, -0.66667, 0.66667), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, -1, 0.66667), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-1, -0.66667, -0.66667), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, -1, -0.66667), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, -1, -0.66667), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, -1), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.66667, -1, -0.66667), glm::vec2(0.4375, 0.5625) });
	verts.push_back({ glm::vec3(1, -0.66667, -0.66667), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, -1), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(1, 0.66667, -0.66667), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, 1), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(-1, -0.66667, 0.66667), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, 1), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, 1), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(1, 0.66667, 0.66667), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, 1), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, -1), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(-1, 0.66667, -0.66667), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, -1), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, 1, 0.66667), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, 1), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(0.66667, 1, 0.66667), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(0.66667, 1, 0.66667), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(1, 0.66667, -0.66667), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(0.66667, 1, -0.66667), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(0.66667, -1, 0.66667), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, 1), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(-0.66667, -1, 0.66667), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(-0.66667, 1, -0.66667), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(-1, 0.66667, 0.66667), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, 1, 0.66667), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(0.66667, 1, -0.66667), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, -1), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(-0.66667, 1, -0.66667), glm::vec2(0.4375, 0.5625) });
	verts.push_back({ glm::vec3(0.66667, -1, -0.66667), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(1, -0.66667, 0.66667), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(0.66667, -1, 0.66667), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, -1), glm::vec2(0.929688, 0.304688) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, -1), glm::vec2(0.695312, 0.070312) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, -1), glm::vec2(0.929688, 0.070312) });
	verts.push_back({ glm::vec3(0.66667, -1, -0.66667), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(0.66667, -1, 0.66667), glm::vec2(0.25, 0.5625) });
	verts.push_back({ glm::vec3(-0.66667, -1, 0.66667), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(-1, 0.66667, 0.66667), glm::vec2(0.304688, 0.304688) });
	verts.push_back({ glm::vec3(-1, 0.66667, -0.66667), glm::vec2(0.070312, 0.304688) });
	verts.push_back({ glm::vec3(-1, -0.66667, -0.66667), glm::vec2(0.070312, 0.070312) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, 1), glm::vec2(0.929688, 0.617188) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, 1), glm::vec2(0.695312, 0.617188) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, 1), glm::vec2(0.695312, 0.382812) });
	verts.push_back({ glm::vec3(-0.66667, 1, -0.66667), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(-0.66667, 1, 0.66667), glm::vec2(0.25, 0.5625) });
	verts.push_back({ glm::vec3(0.66667, 1, 0.66667), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(1, 0.66667, -0.66667), glm::vec2(0.617188, 0.304688) });
	verts.push_back({ glm::vec3(1, 0.66667, 0.66667), glm::vec2(0.382812, 0.304688) });
	verts.push_back({ glm::vec3(1, -0.66667, 0.66667), glm::vec2(0.382812, 0.070312) });
	verts.push_back({ glm::vec3(-0.66667, -1, 0.66667), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-1, -0.66667, 0.66667), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(-1, -0.66667, -0.66667), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(-0.66667, -1, -0.66667), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, -1), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, -1), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(1, -0.66667, -0.66667), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, -1), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, -1), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, 1), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(-1, 0.66667, 0.66667), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(-1, -0.66667, 0.66667), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, 1), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(1, -0.66667, 0.66667), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(1, 0.66667, 0.66667), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, -1), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(-1, -0.66667, -0.66667), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(-1, 0.66667, -0.66667), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, 1, 0.66667), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, 1), glm::vec2(0.4375, 0.5625) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, 1), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(0.66667, 1, 0.66667), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(1, 0.66667, 0.66667), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(1, 0.66667, -0.66667), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(0.66667, -1, 0.66667), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, 1), glm::vec2(0.4375, 0.5625) });
	verts.push_back({ glm::vec3(-0.66667, -0.66667, 1), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(-0.66667, 1, -0.66667), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(-1, 0.66667, -0.66667), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(-1, 0.66667, 0.66667), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(0.66667, 1, -0.66667), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, -1), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, -1), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.66667, -1, -0.66667), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(1, -0.66667, -0.66667), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(1, -0.66667, 0.66667), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-0.66667, 0.66667, -1), glm::vec2(0.929688, 0.304688) });
	verts.push_back({ glm::vec3(0.66667, 0.66667, -1), glm::vec2(0.695312, 0.304688) });
	verts.push_back({ glm::vec3(0.66667, -0.66667, -1), glm::vec2(0.695312, 0.070312) });

	for (auto& v : verts)
		v.data[4] = 1 - v.data[4];
}