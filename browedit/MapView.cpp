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
#include "components/LubRenderer.h"
#include "components/WaterRenderer.h"
#include "components/BillboardRenderer.h"

#include "shaders/GndShader.h"
#include "shaders/WaterShader.h"

#include <browedit/HotkeyRegistry.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Shader.h>
#include <browedit/gl/Vertex.h>
#include <browedit/gl/Texture.h>
#include <browedit/math/Ray.h>
#include <browedit/math/HermiteCurve.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/shaders/SimpleShader.h>
#include <browedit/HotkeyRegistry.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <filesystem>

std::vector<std::vector<glm::vec3>> debugPoints;
std::mutex debugPointMutex;

extern float timeSelected;

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
	if (!skyBoxMesh.vbo)
		skyBoxMesh.init();
	if (!skyDomeMesh.vbo)
		skyDomeMesh.init();
	if (!simpleShader)
		simpleShader = util::ResourceManager<gl::Shader>::load<SimpleShader>();

	for(int i = 0; i < 9; i++)
		gadgetHeight[i].mode = Gadget::Mode::TranslateY;

	gridVbo = new gl::VBO<VertexP3T2>();
	rotateGridVbo = new gl::VBO<VertexP3T2>();
	textureGridVbo = new gl::VBO<VertexP3T2>();
	rebuildObjectModeGrid();
	rebuildObjectRotateModeGrid();
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
				browEdit->toolBarToggleButton("viewGat", viewGatGat ? ICON_GAT_ON : ICON_GAT_OFF, viewGatGat, "View GAT tiles", HotkeyAction::View_GatTiles, browEdit->config.toolbarButtonsViewOptions);
			else
				browEdit->toolBarToggleButton("viewGat", viewGat ? ICON_GAT_ON : ICON_GAT_OFF, viewGat, "View GAT tiles", HotkeyAction::View_GatTiles, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewFog", viewFog ? ICON_VIEW_FOG_ON: ICON_VIEW_FOG_OFF, viewFog, "View Fog", HotkeyAction::View_Fog, browEdit->config.toolbarButtonsViewOptions);

			if (browEdit->editMode == BrowEdit::EditMode::Gat ? viewGatGat : viewGat)
				ImGui::DragFloat("Gat Opacity", &gatOpacity, 0.025f, 0.0f, 1.0f);


			ImGui::Text("Object render settings");
			browEdit->toolBarToggleButton("viewModels", viewModels ? ICON_VIEW_MODEL_ON : ICON_VIEW_MODEL_OFF, viewModels, "View Models", HotkeyAction::View_Models, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewEffects", viewEffects ? ICON_VIEW_EFFECT_ON : ICON_VIEW_EFFECT_OFF, viewEffects, "View Effects", HotkeyAction::View_Effects, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewSounds", viewSounds ? ICON_VIEW_SOUND_ON : ICON_VIEW_SOUND_OFF, viewSounds, "View Sounds", HotkeyAction::View_Sounds, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewLights", viewLights ? ICON_VIEW_LIGHT_ON : ICON_VIEW_LIGHT_OFF, viewLights, "View Lights", HotkeyAction::View_Lights, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewWater", viewWater ? ICON_VIEW_WATER_ON : ICON_VIEW_WATER_OFF, viewWater, "View Water", HotkeyAction::View_Water, browEdit->config.toolbarButtonsViewOptions);
			ImGui::SameLine();
			browEdit->toolBarToggleButton("viewEffectIcons", viewEffectIcons ? ICON_VIEW_EFFECT_ON : ICON_VIEW_EFFECT_ON, viewEffectIcons, "View Effect Icons", HotkeyAction::View_EffectIcons, browEdit->config.toolbarButtonsViewOptions);

			if (browEdit->editMode == BrowEdit::EditMode::Object)
			{
				ImGui::Text("Light render settings");
				browEdit->toolBarToggleButton("showAllLights", showAllLights ? ICON_ALL_LIGHTS_ON : ICON_ALL_LIGHTS_OFF, &showAllLights, "Show all lights", browEdit->config.toolbarButtonsViewOptions);
				ImGui::SameLine();
				browEdit->toolBarToggleButton("showLightSphere", showLightSphere ? ICON_LIGHTSPHERE_ON : ICON_LIGHTSPHERE_OFF, &showLightSphere, "Show light sphere", browEdit->config.toolbarButtonsViewOptions);
			}

			ImGui::Text("Skybox settings");

			std::string skybox = "";
			if(skyTexture)
				skybox = std::filesystem::path(skyTexture->fileName).filename().string();

			if (ImGui::BeginCombo("Skybox", skybox.c_str(), ImGuiComboFlags_HeightLargest))
			{
				if (skyTextures.size() == 0)
				{
					std::vector<std::string> files = util::FileIO::listFiles("data\\texture\\skyboxes");
					for (auto& f : files)
						skyTextures.push_back(util::ResourceManager<gl::Texture>::load(f));
				}
				if (ImGui::Selectable("##empty", !skyTexture, 0, ImVec2(0, 64)))
				{
					if(skyTexture)
						util::ResourceManager<gl::Texture>::unload(skyTexture);
					skyTexture = nullptr;
					for (auto& t : skyTextures)
						if (t != skyTexture)
							util::ResourceManager<gl::Texture>::unload(t);
					skyTextures.clear();
				}
				for (auto i = 0; i < skyTextures.size(); i++)
				{
					if (ImGui::Selectable(("##" + skyTextures[i]->fileName).c_str(), false, 0, ImVec2(0, 64)))
					{
						if (skyTexture)
							util::ResourceManager<gl::Texture>::unload(skyTexture);
						skyTexture = skyTextures[i];
						for (auto& t : skyTextures)
							if(t != skyTexture)
								util::ResourceManager<gl::Texture>::unload(t);
						skyTextures.clear();
						break;
					}
					ImGui::SameLine(0);
					ImGui::Image((ImTextureID)(long long)skyTextures[i]->id(), ImVec2(64, 64));
					ImGui::SameLine(80);
					ImGui::Text(std::filesystem::path(skyTextures[i]->fileName).filename().string().c_str());

				}
				ImGui::EndCombo();
			}



			ImGui::Checkbox("Cube", &skyboxCube);
			ImGui::DragFloat("Height", &skyboxHeight, 10, -4000, 4000);
			ImGui::DragFloat("Rotation", &skyboxRotation, 1, 0, 360);

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
		switch (gadget.mode) {
			case Gadget::Mode::Scale:
				if (ImGui::DragFloat("##gridSize", &gridSizeScale, 1.0f, 0.1f, 100.0f, "%.3f"))
					rebuildObjectModeGrid();
				break;
			case Gadget::Mode::Rotate:
				if (ImGui::DragFloat("##gridSize", &gridSizeRotate, 1.0f, 0.1f, 100.0f, "%.3f"))
					rebuildObjectRotateModeGrid();
				break;
			default:
				if (ImGui::DragFloat("##gridSize", &gridSizeTranslate, 1.0f, 0.1f, 100.0f, "%.3f"))
					rebuildObjectModeGrid();
				break;
		}
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
					else if (gadget.mode == Gadget::Mode::Scale)
						gridSizeScale = f;
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
			if (gadget.mode == Gadget::Mode::Scale)
				ImGui::DragFloat("##gridOffset", &gridOffsetScale, 1.0f, 0, gridSizeScale, "%.2f");
			else if (gadget.mode == Gadget::Mode::Translate)
				ImGui::DragFloat("##gridOffset", &gridOffsetTranslate, 1.0f, 0, gridSizeTranslate, "%.2f");
			else
				if (ImGui::DragFloat("##gridOffset", &gridOffsetRotate, 1.0f, 0, gridSizeRotate, "%.2f"))
					rebuildObjectRotateModeGrid();
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
			HotkeyRegistry::runAction(HotkeyAction::ObjectEdit_Move);
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("rotate", ICON_ROTATE, gadget.mode == Gadget::Mode::Rotate, "Rotate", browEdit->config.toolbarButtonsObjectEdit))
			HotkeyRegistry::runAction(HotkeyAction::ObjectEdit_Rotate);
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("scale", ICON_SCALE, gadget.mode == Gadget::Mode::Scale, "Scale", browEdit->config.toolbarButtonsObjectEdit))
			HotkeyRegistry::runAction(HotkeyAction::ObjectEdit_Scale);


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
	if (ImGui::SliderFloat("##gadgetScale", &gadgetScale, 0, 2)) {
		Gadget::scale = gadgetScale;
		rebuildObjectRotateModeGrid();
	}
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
	auto rsw = map->rootNode->getComponent<Rsw>();
	if(rsw && viewFog)
		glClearColor(rsw->fog.color.r, rsw->fog.color.g, rsw->fog.color.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	float ratio = fbo->getWidth() / (float)fbo->getHeight();

	if (ortho)
		nodeRenderContext.projectionMatrix = glm::ortho(-cameraDistance/2*ratio, cameraDistance/2 * ratio, -cameraDistance/2, cameraDistance/2, -5000.0f, 5000.0f);
	else {
		float nearPlane = 0.1f;

		if (cameraDistance > 500)
			nearPlane = 10.0f;
		else if (cameraDistance > 25)
			nearPlane = 1.0f;
		nodeRenderContext.projectionMatrix = glm::perspective(glm::radians(browEdit->config.fov), ratio, nearPlane, 5000.0f);
	}
	if (cinematicPlay && rsw->cinematicTracks.size() > 0)
	{
		auto beforePos = (Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>*)rsw->cinematicTracks[0].getBeforeFrame(timeSelected);
		auto afterPos = (Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>*)rsw->cinematicTracks[0].getAfterFrame(timeSelected);
		float frameTime = afterPos->time - beforePos->time;
		float mixFactor = (timeSelected - beforePos->time) / frameTime;
		float segmentLength = math::HermiteCurve::getLength(beforePos->data.first, beforePos->data.second, afterPos->data.first, afterPos->data.second);
		glm::vec3 pos = math::HermiteCurve::getPointAtDistance(beforePos->data.first, beforePos->data.second, afterPos->data.first, afterPos->data.second, mixFactor * segmentLength);
		auto afterRot = (Rsw::KeyFrameData<Rsw::CameraTarget>*)rsw->cinematicTracks[1].getAfterFrame(timeSelected);
		auto beforeRot = (Rsw::KeyFrameData<Rsw::CameraTarget>*)rsw->cinematicTracks[1].getBeforeFrame(timeSelected);

		nodeRenderContext.viewMatrix = glm::mat4(1.0f);
		glm::quat targetRot = cinematicCameraDirection;
		glm::quat lastRot = cinematicCameraDirection;
		if (afterRot->data.lookAt == Rsw::CameraTarget::LookAt::Point)
		{
			glm::vec3 direction(afterRot->data.point - pos);
			targetRot = glm::conjugate(glm::quatLookAt(glm::normalize(direction), glm::vec3(0, 1, 0)));
		}
		else if (afterRot->data.lookAt == Rsw::CameraTarget::LookAt::Direction)
		{
			glm::vec3 forward = math::HermiteCurve::getPointAtDistance(beforePos->data.first, beforePos->data.second, afterPos->data.first, afterPos->data.second, (mixFactor+0.01f) * segmentLength);
			if (glm::distance(forward, pos) > 0)
			{
				forward = glm::normalize(forward - pos);
				targetRot = afterRot->data.angle * glm::conjugate(glm::quatLookAt(glm::normalize(forward), glm::vec3(0, 1, 0)));
			}
		}
		else if (afterRot->data.lookAt == Rsw::CameraTarget::LookAt::FixedDirection)
		{
			targetRot = afterRot->data.angle;
		}

		if (beforeRot->data.lookAt == Rsw::CameraTarget::LookAt::Point)
		{
			glm::vec3 direction(beforeRot->data.point - pos);
			lastRot = glm::conjugate(glm::quatLookAt(glm::normalize(direction), glm::vec3(0, 1, 0)));
		}
		else if (beforeRot->data.lookAt == Rsw::CameraTarget::LookAt::Direction)
		{
			//TODO
		}
		else if (beforeRot->data.lookAt == Rsw::CameraTarget::LookAt::FixedDirection)
		{
			lastRot = beforeRot->data.angle;
		}
		if (glm::distance(cinematicLastCameraPosition, pos) > 5)
			cinematicCameraDirection = targetRot;
		else
			cinematicCameraDirection = util::RotateTowards(lastRot, targetRot, afterRot->data.turnSpeed * (timeSelected - beforeRot->time));
		nodeRenderContext.viewMatrix = nodeRenderContext.viewMatrix * glm::toMat4(cinematicCameraDirection);
		nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, -pos);
		cinematicLastCameraPosition = pos;
	}
	else
	{
		nodeRenderContext.viewMatrix = glm::mat4(1.0f);
		nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, glm::vec3(0, 0, -cameraDistance));
		nodeRenderContext.viewMatrix = glm::rotate(nodeRenderContext.viewMatrix, glm::radians(cameraRot.x), glm::vec3(1, 0, 0));
		nodeRenderContext.viewMatrix = glm::rotate(nodeRenderContext.viewMatrix, glm::radians(cameraRot.y), glm::vec3(0, 1, 0));
		nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, -cameraCenter);
	}

	//TODO: fix this, don't want to individually set settings
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	gndRenderer->viewLightmapShadow = viewLightmapShadow;
	gndRenderer->viewLightmapColor = viewLightmapColor;
	gndRenderer->viewColors = viewColors;
	gndRenderer->viewLighting = viewLighting;
	gndRenderer->viewTextures = viewTextures;
	gndRenderer->viewFog = viewFog;

	if (gndRenderer->viewEmptyTiles != viewEmptyTiles)
	{
		gndRenderer->viewEmptyTiles = viewEmptyTiles;
		auto gnd = map->rootNode->getComponent<Gnd>();
		for (int x = 0; x < gnd->width; x++)
			for (int y = 0; y < gnd->height; y++)
				gndRenderer->setChunkDirty(x,y);
	}
	if (gndRenderer->smoothColors != smoothColors)
	{
		gndRenderer->smoothColors = smoothColors;
		gndRenderer->gndShadowDirty = true;
	}

	map->rootNode->getComponent<WaterRenderer>()->viewFog = viewFog;
	
	RsmRenderer::RsmRenderContext::getInstance()->viewLighting = viewLighting;
	RsmRenderer::RsmRenderContext::getInstance()->viewTextures = viewTextures;
	RsmRenderer::RsmRenderContext::getInstance()->viewFog = viewFog;
	RsmRenderer::RsmRenderContext::getInstance()->enableFaceCulling = enableFaceCulling;

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
			{
				if (dynamic_cast<BillboardRenderer*>(r))
				{
					r->enabled = viewEffects;
					r->node->getComponent<BillboardRenderer>()->enabled = viewEffectIcons && viewEffects;
					auto lubRenderer = r->node->getComponent<LubRenderer>();
					if (lubRenderer)
						lubRenderer->enabled = viewEffects;
				}
			}
			if (r->node->getComponent<LubEffect>())
				r->node->getComponent<BillboardRenderer>()->enabled = viewEffectIcons && viewEffects;
			if (r->node->getComponent<RswSound>())
				r->enabled = viewSounds;
			if (r->node->getComponent<RswLight>())
				r->enabled = viewLights;
		}
	}
	map->rootNode->getComponent<WaterRenderer>()->enabled = viewWater;

	NodeRenderer::render(map->rootNode, nodeRenderContext);

	// position and draw the new nodes
	if (browEdit->newNodes.size() > 0 && hovered)
	{
		auto gnd = map->rootNode->getComponent<Gnd>();
		auto rayCast = gnd->rayCast(mouseRay);
		if (rayCast != glm::vec3(std::numeric_limits<float>().max()))
		{
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
				auto rsm = newNode.first->getComponent<Rsm>();

				switch (browEdit->newNodePlacement) {
					case BrowEdit::Relative:
						rswObject->position.y += newNode.second.y + browEdit->newNodesCenter.y;
						break;
					case BrowEdit::Absolute:
						rswObject->position.y = newNode.second.y + browEdit->newNodesCenter.y;
						break;
					case BrowEdit::Ground:
						if (rsm && rsm->version >= 0x202) {
							rswObject->position.y += rsm->bbmin.y;
						}
						break;
				}

				auto rsmRenderer = newNode.first->getComponent<RsmRenderer>();
				if (rsmRenderer)
				{

					glm::mat4 matrix = glm::mat4(1.0f);
					matrix = glm::scale(matrix, glm::vec3(1, 1, -1));
					matrix = glm::translate(matrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));
					matrix = glm::rotate(matrix, -glm::radians(rswObject->rotation.z), glm::vec3(0, 0, 1));
					matrix = glm::rotate(matrix, -glm::radians(rswObject->rotation.x), glm::vec3(1, 0, 0));
					matrix = glm::rotate(matrix, glm::radians(rswObject->rotation.y), glm::vec3(0, 1, 0));

					if (rsm) {
						matrix = glm::scale(matrix, glm::vec3(rswObject->scale.x, -rswObject->scale.y, rswObject->scale.z));

						if (rsm->version < 0x202) {
							matrix = glm::translate(matrix, glm::vec3(-rsm->realbbrange.x, rsm->realbbmin.y, -rsm->realbbrange.z));
						}
						else {
							matrix = glm::scale(matrix, glm::vec3(1, -1, 1));
						}
					}

					rsmRenderer->matrixCache = matrix;
					rsmRenderer->reverseCullFace = false;

					if (rswObject->scale.x * rswObject->scale.y * rswObject->scale.z * (rsm->version >= 0x202 ? -1 : 1) < 0)
						rsmRenderer->reverseCullFace = true;
				}
				if (newNode.first->getComponent<BillboardRenderer>())
					newNode.first->getComponent<BillboardRenderer>()->gnd = gnd;

				// If matrixCached is not set to true, the renderer assumes it's being called from Bromedit
				if (rsmRenderer)
					rsmRenderer->matrixCached = true;
				NodeRenderer::render(newNode.first, nodeRenderContext);
				if (rsmRenderer)
					rsmRenderer->matrixCached = false;
			}
		}
	}
	if (skyTexture)
	{
		simpleShader->use();
		glm::mat4 vm = nodeRenderContext.viewMatrix;
		vm[3][0] = 0;	vm[3][1] = 0;	vm[3][2] = 0;	vm[3][3] = 1;
		simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
		simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, vm);
		simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::scale(glm::translate(glm::rotate(glm::mat4(1.0f), glm::radians(skyboxRotation), glm::vec3(0,1,0)), glm::vec3(0, skyboxHeight, 0)), glm::vec3(3500.0f)));
		simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
		simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
		skyTexture->bind();

		if(skyboxCube)
			skyBoxMesh.draw();
		else
			skyDomeMesh.draw();
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
		cameraTargetRot.x = glm::clamp(cameraTargetRot.x, 0.0f, 90.0f);
		auto diff = cameraTargetRot - cameraRot;
		diff.x = util::wrap360(diff.x);
		diff.y = util::wrap360(diff.y);

		cameraRot += 180 * deltaTime * glm::sign(diff);
	
		if(glm::distance(cameraCenter, cameraTargetPos) < 2000.0f * deltaTime)
			cameraCenter = cameraTargetPos;
		else
			cameraCenter -= glm::normalize(cameraCenter - cameraTargetPos) * 2000.0f * deltaTime;

		if (glm::abs(diff.x) < 180 * deltaTime && glm::abs(diff.y) < 180 * deltaTime)
		{
			cameraRot = cameraTargetRot;
		}

		if (glm::abs(diff.x) < 180 * deltaTime && glm::abs(diff.y) < 180 * deltaTime && glm::distance(cameraCenter, cameraTargetPos) < 2000.0f * deltaTime)
			cameraAnimating = false;
	}

	if (hovered)
	{
		if ((mouseState.buttons&6) != 0)
		{
			ImGui::Begin("Statusbar");
			browEdit->statusText = true;
			ImGui::Text("Camera: shift to rotate, ctrl to go up/down, ctrl+shift to zoom");
			ImGui::End();
			if (ImGui::GetIO().KeyShift && ImGui::GetIO().KeyCtrl)
			{
				cameraDistance *= 1.0f + (mouseState.position.y - prevMouseState.position.y) * 0.0025f * browEdit->config.cameraMouseSpeed;
			}
			else if (ImGui::GetIO().KeyShift)
			{
				cameraRot.x += (mouseState.position.y - prevMouseState.position.y) * 0.25f * browEdit->config.cameraMouseSpeed;
				cameraRot.y += (mouseState.position.x - prevMouseState.position.x) * 0.25f * browEdit->config.cameraMouseSpeed;
				cameraRot.x = glm::clamp(cameraRot.x, 0.0f, 90.0f);
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
					* glm::rotate(glm::mat4(1.0f), glm::radians(cameraRot.y), glm::vec3(0, 1, 0)));

				auto rayCast = map->rootNode->getComponent<Gnd>()->rayCast(math::Ray(cameraCenter + glm::vec3(0,9999,0), glm::vec3(0,-1,0)), viewEmptyTiles);
				if (rayCast != glm::vec3(std::numeric_limits<float>().max()))
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
	viewMatrix = glm::rotate(viewMatrix, glm::radians(cameraRot.x), glm::vec3(1, 0, 0));
	viewMatrix = glm::rotate(viewMatrix, glm::radians(cameraRot.y), glm::vec3(0, 1, 0));
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
		float t;
		if (cubeMesh.collision(ray, glm::mat4(1.0f), t))
		{
			if (ImGui::IsMouseClicked(0))
			{
				glm::vec3 point = cubeMesh.getCollision(ray, glm::mat4(1.0f));
				//std::cout << point.x << "\t" << point.y << "\t" << point.z << std::endl;
				if (glm::abs(point.y - 1) < 0.001)
				{
					if (cameraRot.x == 90)
					{
						float d = cameraRot.y / 90;
						while (d >= 1)
							d--;
						if (glm::abs(d) < 0.01)
							cameraTargetRot = cameraRot + glm::vec2(0, 90);
						else
							cameraTargetRot = glm::vec2(90, 0);
					}
					else
						cameraTargetRot = glm::vec2(90, 0);
					cameraTargetPos = cameraCenter;
					cameraAnimating = true;
				}
				else if (glm::abs(point.z) - 1 < 0.001 || glm::abs(point.x) - 1 < 0.001) //side
				{
					float angle = std::atan2(glm::round(point.z), glm::round(point.x));
					cameraTargetRot = glm::vec2(0, glm::degrees(angle)-90.0f);
					if (glm::abs(point.y-1) < 0.25f)
						cameraTargetRot.x = 45;
					cameraTargetPos = cameraCenter;
					cameraAnimating = true;
				}
				if (ImGui::IsWindowHovered())
					return true;
			}
			if (ImGui::IsWindowHovered())
				return true;
		}
	}
	return false;
}


void MapView::focusSelection()
{
	cameraAnimating = true;
	cameraTargetRot = cameraRot;
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

	if (rswLight->lightType == RswLight::Type::Sun || gnd == nullptr)
		return;

	glm::mat4 modelMatrix(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1, 1, -1));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));
	if (rswLight->lightType == RswLight::Type::Point)
	{
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
	else if (rswLight->lightType == RswLight::Type::Spot)
	{
		auto mm = modelMatrix;
		mm = mm * glm::toMat4(util::RotationBetweenVectors(glm::vec3(1, 0, 0), rswLight->direction * glm::vec3(1,1,-1)));

		simpleShader->use();
		simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
		simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
		simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, mm);
		simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(rswLight->color, 0.10f));
		glDepthMask(0);
		glEnable(GL_BLEND);
		
		std::vector<VertexP3T2N3> verts;
		constexpr float inc = glm::pi<float>() / 10.0f;

		float height = rswLight->range;
		float width = rswLight->range * tan(glm::acos(1-rswLight->spotlightWidth));

		for (float f = 0; f < 2 * glm::pi<float>(); f += inc)
		{
			verts.push_back(VertexP3T2N3(glm::vec3(0, 0, 0), glm::vec2(0), glm::vec3(1.0f)));
			verts.push_back(VertexP3T2N3(glm::vec3(height, width * glm::sin(f), width * glm::cos(f)), glm::vec2(0), glm::vec3(1.0f)));
			verts.push_back(VertexP3T2N3(glm::vec3(height, width * glm::sin(f+inc), width * glm::cos(f+inc)), glm::vec2(0), glm::vec3(1.0f)));
		}

		if (showLightSphere) {
			for (float f = 0; f < 2 * glm::pi<float>(); f += inc)
			{
				verts.push_back(VertexP3T2N3(glm::vec3(height, 0, 0), glm::vec2(0), glm::vec3(1.0f)));
				verts.push_back(VertexP3T2N3(glm::vec3(height, width * glm::sin(f), width * glm::cos(f)), glm::vec2(0), glm::vec3(1.0f)));
				verts.push_back(VertexP3T2N3(glm::vec3(height, width * glm::sin(f + inc), width * glm::cos(f + inc)), glm::vec2(0), glm::vec3(1.0f)));
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4); //TODO: vao
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);

		glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());

		if (!showLightSphere) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(1);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(rswLight->color, 0.8f));
			glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glDepthMask(1);

		if (!showLightSphere) {
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(rswLight->color, 1.0f));
			verts.clear();
		
			for (float f = 0; f < 2 * glm::pi<float>(); f += inc)
			{
				verts.push_back(VertexP3T2N3(glm::vec3(height, width * glm::sin(f), width * glm::cos(f)), glm::vec2(0), glm::vec3(1.0f)));
				verts.push_back(VertexP3T2N3(glm::vec3(height, width * glm::sin(f + inc), width * glm::cos(f + inc)), glm::vec2(0), glm::vec3(1.0f)));
			}

			glLineWidth(2);
			glDrawArrays(GL_LINES, 0, (int)verts.size());
		}

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


void MapView::SkyBoxMesh::buildVertices(std::vector<VertexP3T2N3>& verts)
{
	verts.push_back({ glm::vec3(-1, 1, -1), glm::vec2(0.5, 1) });
	verts.push_back({ glm::vec3(1, 1, 1), glm::vec2(0.25, 0.666667) });
	verts.push_back({ glm::vec3(1, 1, -1), glm::vec2(0.5, 0.666667) });
	verts.push_back({ glm::vec3(1, 1, 1), glm::vec2(0.25, 0.666667) });
	verts.push_back({ glm::vec3(-1, -1, 1), glm::vec2(0, 0.333333) });
	verts.push_back({ glm::vec3(1, -1, 1), glm::vec2(0.25, 0.333333) });
	verts.push_back({ glm::vec3(-1, 1, 1), glm::vec2(1, 0.666667) });
	verts.push_back({ glm::vec3(-1, -1, -1), glm::vec2(0.75, 0.333333) });
	verts.push_back({ glm::vec3(-1, -1, 1), glm::vec2(1, 0.333333) });
	verts.push_back({ glm::vec3(1, -1, -1), glm::vec2(0.5, 0.333333) });
	verts.push_back({ glm::vec3(-1, -1, 1), glm::vec2(0.25, 0) });
	verts.push_back({ glm::vec3(-1, -1, -1), glm::vec2(0.5, 0) });
	verts.push_back({ glm::vec3(1, 1, -1), glm::vec2(0.5, 0.666667) });
	verts.push_back({ glm::vec3(1, -1, 1), glm::vec2(0.25, 0.333333) });
	verts.push_back({ glm::vec3(1, -1, -1), glm::vec2(0.5, 0.333333) });
	verts.push_back({ glm::vec3(-1, 1, -1), glm::vec2(0.75, 0.666667) });
	verts.push_back({ glm::vec3(1, -1, -1), glm::vec2(0.5, 0.333333) });
	verts.push_back({ glm::vec3(-1, -1, -1), glm::vec2(0.75, 0.333333) });
	verts.push_back({ glm::vec3(-1, 1, -1), glm::vec2(0.5, 1) });
	verts.push_back({ glm::vec3(-1, 1, 1), glm::vec2(0.25, 1) });
	verts.push_back({ glm::vec3(1, 1, 1), glm::vec2(0.25, 0.666667) });
	verts.push_back({ glm::vec3(1, 1, 1), glm::vec2(0.25, 0.666667) });
	verts.push_back({ glm::vec3(-1, 1, 1), glm::vec2(0, 0.666667) });
	verts.push_back({ glm::vec3(-1, -1, 1), glm::vec2(0, 0.333333) });
	verts.push_back({ glm::vec3(-1, 1, 1), glm::vec2(1, 0.666667) });
	verts.push_back({ glm::vec3(-1, 1, -1), glm::vec2(0.75, 0.666667) });
	verts.push_back({ glm::vec3(-1, -1, -1), glm::vec2(0.75, 0.333333) });
	verts.push_back({ glm::vec3(1, -1, -1), glm::vec2(0.5, 0.333333) });
	verts.push_back({ glm::vec3(1, -1, 1), glm::vec2(0.25, 0.333333) });
	verts.push_back({ glm::vec3(-1, -1, 1), glm::vec2(0.25, 0) });
	verts.push_back({ glm::vec3(1, 1, -1), glm::vec2(0.5, 0.666667) });
	verts.push_back({ glm::vec3(1, 1, 1), glm::vec2(0.25, 0.666667) });
	verts.push_back({ glm::vec3(1, -1, 1), glm::vec2(0.25, 0.333333) });
	verts.push_back({ glm::vec3(-1, 1, -1), glm::vec2(0.75, 0.666667) });
	verts.push_back({ glm::vec3(1, 1, -1), glm::vec2(0.5, 0.666667) });
	verts.push_back({ glm::vec3(1, -1, -1), glm::vec2(0.5, 0.333333) });

	for (auto& v : verts)
		v.data[4] = 1 - v.data[4];
}



void MapView::SkyDomeMesh::buildVertices(std::vector<VertexP3T2N3>& verts)
{
	verts.push_back({ glm::vec3(0, 0.55557, -0.83147), glm::vec2(0.75, 0.6875) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, -0.69352), glm::vec2(0.71875, 0.75) });
	verts.push_back({ glm::vec3(0.162212, 0.55557, -0.815493), glm::vec2(0.71875, 0.6875) });
	verts.push_back({ glm::vec3(0, -0.707107, -0.707107), glm::vec2(0.75, 0.25) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, -0.544895), glm::vec2(0.71875, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.83147, -0.55557), glm::vec2(0.75, 0.1875) });
	verts.push_back({ glm::vec3(0, 0.382683, -0.923879), glm::vec2(0.75, 0.625) });
	verts.push_back({ glm::vec3(0.162212, 0.55557, -0.815493), glm::vec2(0.71875, 0.6875) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, -0.906127), glm::vec2(0.71875, 0.625) });
	verts.push_back({ glm::vec3(0, -0.83147, -0.55557), glm::vec2(0.75, 0.1875) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, -0.37533), glm::vec2(0.71875, 0.125) });
	verts.push_back({ glm::vec3(0, -0.92388, -0.382683), glm::vec2(0.75, 0.125) });
	verts.push_back({ glm::vec3(0, 0.382683, -0.923879), glm::vec2(0.75, 0.625) });
	verts.push_back({ glm::vec3(0.191342, 0.19509, -0.96194), glm::vec2(0.71875, 0.5625) });
	verts.push_back({ glm::vec3(0, 0.19509, -0.980785), glm::vec2(0.75, 0.5625) });
	verts.push_back({ glm::vec3(0, -0.92388, -0.382683), glm::vec2(0.75, 0.125) });
	verts.push_back({ glm::vec3(0.03806, -0.980785, -0.191342), glm::vec2(0.71875, 0.0625) });
	verts.push_back({ glm::vec3(0, -0.980785, -0.19509), glm::vec2(0.75, 0.0625) });
	verts.push_back({ glm::vec3(0, 0.19509, -0.980785), glm::vec2(0.75, 0.5625) });
	verts.push_back({ glm::vec3(0.19509, 0, -0.980785), glm::vec2(0.71875, 0.5) });
	verts.push_back({ glm::vec3(0, 0, -1), glm::vec2(0.75, 0.5) });
	verts.push_back({ glm::vec3(0, 0.980785, -0.19509), glm::vec2(0.75, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.734375, 1) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, -0.191342), glm::vec2(0.71875, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.734375, 0) });
	verts.push_back({ glm::vec3(0, -0.980785, -0.19509), glm::vec2(0.75, 0.0625) });
	verts.push_back({ glm::vec3(0.03806, -0.980785, -0.191342), glm::vec2(0.71875, 0.0625) });
	verts.push_back({ glm::vec3(0, -0.19509, -0.980785), glm::vec2(0.75, 0.4375) });
	verts.push_back({ glm::vec3(0.19509, 0, -0.980785), glm::vec2(0.71875, 0.5) });
	verts.push_back({ glm::vec3(0.191342, -0.19509, -0.96194), glm::vec2(0.71875, 0.4375) });
	verts.push_back({ glm::vec3(0, 0.92388, -0.382683), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, -0.191342), glm::vec2(0.71875, 0.9375) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, -0.37533), glm::vec2(0.71875, 0.875) });
	verts.push_back({ glm::vec3(0, -0.19509, -0.980785), glm::vec2(0.75, 0.4375) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, -0.906127), glm::vec2(0.71875, 0.375) });
	verts.push_back({ glm::vec3(0, -0.382683, -0.923879), glm::vec2(0.75, 0.375) });
	verts.push_back({ glm::vec3(0, 0.83147, -0.55557), glm::vec2(0.75, 0.8125) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, -0.37533), glm::vec2(0.71875, 0.875) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, -0.544895), glm::vec2(0.71875, 0.8125) });
	verts.push_back({ glm::vec3(0, -0.382683, -0.923879), glm::vec2(0.75, 0.375) });
	verts.push_back({ glm::vec3(0.162212, -0.55557, -0.815493), glm::vec2(0.71875, 0.3125) });
	verts.push_back({ glm::vec3(0, -0.55557, -0.83147), glm::vec2(0.75, 0.3125) });
	verts.push_back({ glm::vec3(0, 0.83147, -0.55557), glm::vec2(0.75, 0.8125) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, -0.69352), glm::vec2(0.71875, 0.75) });
	verts.push_back({ glm::vec3(0, 0.707107, -0.707107), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(0, -0.55557, -0.83147), glm::vec2(0.75, 0.3125) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, -0.69352), glm::vec2(0.71875, 0.25) });
	verts.push_back({ glm::vec3(0, -0.707107, -0.707107), glm::vec2(0.75, 0.25) });
	verts.push_back({ glm::vec3(0.162212, -0.55557, -0.815493), glm::vec2(0.71875, 0.3125) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, -0.853553), glm::vec2(0.6875, 0.375) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, -0.768178), glm::vec2(0.6875, 0.3125) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, -0.544895), glm::vec2(0.71875, 0.8125) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, -0.653281), glm::vec2(0.6875, 0.75) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, -0.69352), glm::vec2(0.71875, 0.75) });
	verts.push_back({ glm::vec3(0.162212, -0.55557, -0.815493), glm::vec2(0.71875, 0.3125) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, -0.653281), glm::vec2(0.6875, 0.25) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, -0.69352), glm::vec2(0.71875, 0.25) });
	verts.push_back({ glm::vec3(0.162212, 0.55557, -0.815493), glm::vec2(0.71875, 0.6875) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, -0.653281), glm::vec2(0.6875, 0.75) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, -0.768178), glm::vec2(0.6875, 0.6875) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, -0.544895), glm::vec2(0.71875, 0.1875) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, -0.653281), glm::vec2(0.6875, 0.25) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, -0.51328), glm::vec2(0.6875, 0.1875) });
	verts.push_back({ glm::vec3(0.162212, 0.55557, -0.815493), glm::vec2(0.71875, 0.6875) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, -0.853553), glm::vec2(0.6875, 0.625) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, -0.906127), glm::vec2(0.71875, 0.625) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, -0.37533), glm::vec2(0.71875, 0.125) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, -0.51328), glm::vec2(0.6875, 0.1875) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, -0.353553), glm::vec2(0.6875, 0.125) });
	verts.push_back({ glm::vec3(0.191342, 0.19509, -0.96194), glm::vec2(0.71875, 0.5625) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, -0.853553), glm::vec2(0.6875, 0.625) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, -0.906127), glm::vec2(0.6875, 0.5625) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, -0.37533), glm::vec2(0.71875, 0.125) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, -0.18024), glm::vec2(0.6875, 0.0625) });
	verts.push_back({ glm::vec3(0.03806, -0.980785, -0.191342), glm::vec2(0.71875, 0.0625) });
	verts.push_back({ glm::vec3(0.191342, 0.19509, -0.96194), glm::vec2(0.71875, 0.5625) });
	verts.push_back({ glm::vec3(0.382683, 0, -0.923879), glm::vec2(0.6875, 0.5) });
	verts.push_back({ glm::vec3(0.19509, 0, -0.980785), glm::vec2(0.71875, 0.5) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, -0.191342), glm::vec2(0.71875, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.703125, 1) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, -0.18024), glm::vec2(0.6875, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.703125, 0) });
	verts.push_back({ glm::vec3(0.03806, -0.980785, -0.191342), glm::vec2(0.71875, 0.0625) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, -0.18024), glm::vec2(0.6875, 0.0625) });
	verts.push_back({ glm::vec3(0.191342, -0.19509, -0.96194), glm::vec2(0.71875, 0.4375) });
	verts.push_back({ glm::vec3(0.382683, 0, -0.923879), glm::vec2(0.6875, 0.5) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, -0.906127), glm::vec2(0.6875, 0.4375) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, -0.37533), glm::vec2(0.71875, 0.875) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, -0.18024), glm::vec2(0.6875, 0.9375) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, -0.353553), glm::vec2(0.6875, 0.875) });
	verts.push_back({ glm::vec3(0.191342, -0.19509, -0.96194), glm::vec2(0.71875, 0.4375) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, -0.853553), glm::vec2(0.6875, 0.375) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, -0.906127), glm::vec2(0.71875, 0.375) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, -0.37533), glm::vec2(0.71875, 0.875) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, -0.51328), glm::vec2(0.6875, 0.8125) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, -0.544895), glm::vec2(0.71875, 0.8125) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, -0.18024), glm::vec2(0.6875, 0.0625) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, -0.31819), glm::vec2(0.65625, 0.125) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, -0.162212), glm::vec2(0.65625, 0.0625) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, -0.906127), glm::vec2(0.6875, 0.5625) });
	verts.push_back({ glm::vec3(0.55557, 0, -0.831469), glm::vec2(0.65625, 0.5) });
	verts.push_back({ glm::vec3(0.382683, 0, -0.923879), glm::vec2(0.6875, 0.5) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, -0.18024), glm::vec2(0.6875, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.671875, 1) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, -0.162212), glm::vec2(0.65625, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.671875, 0) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, -0.18024), glm::vec2(0.6875, 0.0625) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, -0.162212), glm::vec2(0.65625, 0.0625) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, -0.906127), glm::vec2(0.6875, 0.4375) });
	verts.push_back({ glm::vec3(0.55557, 0, -0.831469), glm::vec2(0.65625, 0.5) });
	verts.push_back({ glm::vec3(0.544895, -0.19509, -0.815493), glm::vec2(0.65625, 0.4375) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, -0.18024), glm::vec2(0.6875, 0.9375) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, -0.31819), glm::vec2(0.65625, 0.875) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, -0.353553), glm::vec2(0.6875, 0.875) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, -0.906127), glm::vec2(0.6875, 0.4375) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, -0.768178), glm::vec2(0.65625, 0.375) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, -0.853553), glm::vec2(0.6875, 0.375) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, -0.353553), glm::vec2(0.6875, 0.875) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, -0.46194), glm::vec2(0.65625, 0.8125) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, -0.51328), glm::vec2(0.6875, 0.8125) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, -0.768178), glm::vec2(0.6875, 0.3125) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, -0.768178), glm::vec2(0.65625, 0.375) });
	verts.push_back({ glm::vec3(0.46194, -0.55557, -0.691342), glm::vec2(0.65625, 0.3125) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, -0.653281), glm::vec2(0.6875, 0.75) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, -0.46194), glm::vec2(0.65625, 0.8125) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, -0.587938), glm::vec2(0.65625, 0.75) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, -0.768178), glm::vec2(0.6875, 0.3125) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, -0.587938), glm::vec2(0.65625, 0.25) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, -0.653281), glm::vec2(0.6875, 0.25) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, -0.768178), glm::vec2(0.6875, 0.6875) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, -0.587938), glm::vec2(0.65625, 0.75) });
	verts.push_back({ glm::vec3(0.46194, 0.55557, -0.691342), glm::vec2(0.65625, 0.6875) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, -0.653281), glm::vec2(0.6875, 0.25) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, -0.46194), glm::vec2(0.65625, 0.1875) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, -0.51328), glm::vec2(0.6875, 0.1875) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, -0.768178), glm::vec2(0.6875, 0.6875) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, -0.768178), glm::vec2(0.65625, 0.625) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, -0.853553), glm::vec2(0.6875, 0.625) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, -0.353553), glm::vec2(0.6875, 0.125) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, -0.46194), glm::vec2(0.65625, 0.1875) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, -0.31819), glm::vec2(0.65625, 0.125) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, -0.906127), glm::vec2(0.6875, 0.5625) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, -0.768178), glm::vec2(0.65625, 0.625) });
	verts.push_back({ glm::vec3(0.544895, 0.19509, -0.815493), glm::vec2(0.65625, 0.5625) });
	verts.push_back({ glm::vec3(0.46194, -0.55557, -0.691342), glm::vec2(0.65625, 0.3125) });
	verts.push_back({ glm::vec3(0.5, -0.707107, -0.5), glm::vec2(0.625, 0.25) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, -0.587938), glm::vec2(0.65625, 0.25) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, -0.587938), glm::vec2(0.65625, 0.75) });
	verts.push_back({ glm::vec3(0.587938, 0.55557, -0.587938), glm::vec2(0.625, 0.6875) });
	verts.push_back({ glm::vec3(0.46194, 0.55557, -0.691342), glm::vec2(0.65625, 0.6875) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, -0.587938), glm::vec2(0.65625, 0.25) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, -0.392847), glm::vec2(0.625, 0.1875) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, -0.46194), glm::vec2(0.65625, 0.1875) });
	verts.push_back({ glm::vec3(0.46194, 0.55557, -0.691342), glm::vec2(0.65625, 0.6875) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, -0.653281), glm::vec2(0.625, 0.625) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, -0.768178), glm::vec2(0.65625, 0.625) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, -0.46194), glm::vec2(0.65625, 0.1875) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, -0.270598), glm::vec2(0.625, 0.125) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, -0.31819), glm::vec2(0.65625, 0.125) });
	verts.push_back({ glm::vec3(0.544895, 0.19509, -0.815493), glm::vec2(0.65625, 0.5625) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, -0.653281), glm::vec2(0.625, 0.625) });
	verts.push_back({ glm::vec3(0.69352, 0.19509, -0.69352), glm::vec2(0.625, 0.5625) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, -0.162212), glm::vec2(0.65625, 0.0625) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, -0.270598), glm::vec2(0.625, 0.125) });
	verts.push_back({ glm::vec3(0.137949, -0.980785, -0.13795), glm::vec2(0.625, 0.0625) });
	verts.push_back({ glm::vec3(0.544895, 0.19509, -0.815493), glm::vec2(0.65625, 0.5625) });
	verts.push_back({ glm::vec3(0.707106, 0, -0.707107), glm::vec2(0.625, 0.5) });
	verts.push_back({ glm::vec3(0.55557, 0, -0.831469), glm::vec2(0.65625, 0.5) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, -0.162212), glm::vec2(0.65625, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.640625, 1) });
	verts.push_back({ glm::vec3(0.137949, 0.980785, -0.13795), glm::vec2(0.625, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.640625, 0) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, -0.162212), glm::vec2(0.65625, 0.0625) });
	verts.push_back({ glm::vec3(0.137949, -0.980785, -0.13795), glm::vec2(0.625, 0.0625) });
	verts.push_back({ glm::vec3(0.544895, -0.19509, -0.815493), glm::vec2(0.65625, 0.4375) });
	verts.push_back({ glm::vec3(0.707106, 0, -0.707107), glm::vec2(0.625, 0.5) });
	verts.push_back({ glm::vec3(0.69352, -0.19509, -0.69352), glm::vec2(0.625, 0.4375) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, -0.162212), glm::vec2(0.65625, 0.9375) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, -0.270598), glm::vec2(0.625, 0.875) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, -0.31819), glm::vec2(0.65625, 0.875) });
	verts.push_back({ glm::vec3(0.544895, -0.19509, -0.815493), glm::vec2(0.65625, 0.4375) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, -0.653281), glm::vec2(0.625, 0.375) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, -0.768178), glm::vec2(0.65625, 0.375) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, -0.46194), glm::vec2(0.65625, 0.8125) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, -0.270598), glm::vec2(0.625, 0.875) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, -0.392847), glm::vec2(0.625, 0.8125) });
	verts.push_back({ glm::vec3(0.46194, -0.55557, -0.691342), glm::vec2(0.65625, 0.3125) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, -0.653281), glm::vec2(0.625, 0.375) });
	verts.push_back({ glm::vec3(0.587938, -0.55557, -0.587938), glm::vec2(0.625, 0.3125) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, -0.587938), glm::vec2(0.65625, 0.75) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, -0.392847), glm::vec2(0.625, 0.8125) });
	verts.push_back({ glm::vec3(0.5, 0.707107, -0.5), glm::vec2(0.625, 0.75) });
	verts.push_back({ glm::vec3(0.137949, 0.980785, -0.13795), glm::vec2(0.625, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.609375, 1) });
	verts.push_back({ glm::vec3(0.162212, 0.980785, -0.108386), glm::vec2(0.59375, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.609375, 0) });
	verts.push_back({ glm::vec3(0.137949, -0.980785, -0.13795), glm::vec2(0.625, 0.0625) });
	verts.push_back({ glm::vec3(0.162212, -0.980785, -0.108386), glm::vec2(0.59375, 0.0625) });
	verts.push_back({ glm::vec3(0.707106, 0, -0.707107), glm::vec2(0.625, 0.5) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, -0.544895), glm::vec2(0.59375, 0.4375) });
	verts.push_back({ glm::vec3(0.69352, -0.19509, -0.69352), glm::vec2(0.625, 0.4375) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, -0.270598), glm::vec2(0.625, 0.875) });
	verts.push_back({ glm::vec3(0.162212, 0.980785, -0.108386), glm::vec2(0.59375, 0.9375) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, -0.212608), glm::vec2(0.59375, 0.875) });
	verts.push_back({ glm::vec3(0.69352, -0.19509, -0.69352), glm::vec2(0.625, 0.4375) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, -0.51328), glm::vec2(0.59375, 0.375) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, -0.653281), glm::vec2(0.625, 0.375) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, -0.392847), glm::vec2(0.625, 0.8125) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, -0.212608), glm::vec2(0.59375, 0.875) });
	verts.push_back({ glm::vec3(0.46194, 0.83147, -0.308658), glm::vec2(0.59375, 0.8125) });
	verts.push_back({ glm::vec3(0.587938, -0.55557, -0.587938), glm::vec2(0.625, 0.3125) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, -0.51328), glm::vec2(0.59375, 0.375) });
	verts.push_back({ glm::vec3(0.691342, -0.55557, -0.46194), glm::vec2(0.59375, 0.3125) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, -0.392847), glm::vec2(0.625, 0.8125) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, -0.392847), glm::vec2(0.59375, 0.75) });
	verts.push_back({ glm::vec3(0.5, 0.707107, -0.5), glm::vec2(0.625, 0.75) });
	verts.push_back({ glm::vec3(0.587938, -0.55557, -0.587938), glm::vec2(0.625, 0.3125) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, -0.392847), glm::vec2(0.59375, 0.25) });
	verts.push_back({ glm::vec3(0.5, -0.707107, -0.5), glm::vec2(0.625, 0.25) });
	verts.push_back({ glm::vec3(0.587938, 0.55557, -0.587938), glm::vec2(0.625, 0.6875) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, -0.392847), glm::vec2(0.59375, 0.75) });
	verts.push_back({ glm::vec3(0.691342, 0.55557, -0.46194), glm::vec2(0.59375, 0.6875) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, -0.392847), glm::vec2(0.625, 0.1875) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, -0.392847), glm::vec2(0.59375, 0.25) });
	verts.push_back({ glm::vec3(0.46194, -0.83147, -0.308658), glm::vec2(0.59375, 0.1875) });
	verts.push_back({ glm::vec3(0.587938, 0.55557, -0.587938), glm::vec2(0.625, 0.6875) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, -0.51328), glm::vec2(0.59375, 0.625) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, -0.653281), glm::vec2(0.625, 0.625) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, -0.392847), glm::vec2(0.625, 0.1875) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, -0.212608), glm::vec2(0.59375, 0.125) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, -0.270598), glm::vec2(0.625, 0.125) });
	verts.push_back({ glm::vec3(0.69352, 0.19509, -0.69352), glm::vec2(0.625, 0.5625) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, -0.51328), glm::vec2(0.59375, 0.625) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, -0.544895), glm::vec2(0.59375, 0.5625) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, -0.270598), glm::vec2(0.625, 0.125) });
	verts.push_back({ glm::vec3(0.162212, -0.980785, -0.108386), glm::vec2(0.59375, 0.0625) });
	verts.push_back({ glm::vec3(0.137949, -0.980785, -0.13795), glm::vec2(0.625, 0.0625) });
	verts.push_back({ glm::vec3(0.69352, 0.19509, -0.69352), glm::vec2(0.625, 0.5625) });
	verts.push_back({ glm::vec3(0.831469, 0, -0.55557), glm::vec2(0.59375, 0.5) });
	verts.push_back({ glm::vec3(0.707106, 0, -0.707107), glm::vec2(0.625, 0.5) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, -0.392847), glm::vec2(0.59375, 0.75) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, -0.31819), glm::vec2(0.5625, 0.6875) });
	verts.push_back({ glm::vec3(0.691342, 0.55557, -0.46194), glm::vec2(0.59375, 0.6875) });
	verts.push_back({ glm::vec3(0.46194, -0.83147, -0.308658), glm::vec2(0.59375, 0.1875) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, -0.270598), glm::vec2(0.5625, 0.25) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, -0.212607), glm::vec2(0.5625, 0.1875) });
	verts.push_back({ glm::vec3(0.691342, 0.55557, -0.46194), glm::vec2(0.59375, 0.6875) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, -0.353553), glm::vec2(0.5625, 0.625) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, -0.51328), glm::vec2(0.59375, 0.625) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, -0.212608), glm::vec2(0.59375, 0.125) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, -0.212607), glm::vec2(0.5625, 0.1875) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, -0.146447), glm::vec2(0.5625, 0.125) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, -0.544895), glm::vec2(0.59375, 0.5625) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, -0.353553), glm::vec2(0.5625, 0.625) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, -0.37533), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.162212, -0.980785, -0.108386), glm::vec2(0.59375, 0.0625) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, -0.146447), glm::vec2(0.5625, 0.125) });
	verts.push_back({ glm::vec3(0.18024, -0.980785, -0.074658), glm::vec2(0.5625, 0.0625) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, -0.544895), glm::vec2(0.59375, 0.5625) });
	verts.push_back({ glm::vec3(0.923879, 0, -0.382683), glm::vec2(0.5625, 0.5) });
	verts.push_back({ glm::vec3(0.831469, 0, -0.55557), glm::vec2(0.59375, 0.5) });
	verts.push_back({ glm::vec3(0.162212, 0.980785, -0.108386), glm::vec2(0.59375, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.578125, 1) });
	verts.push_back({ glm::vec3(0.18024, 0.980785, -0.074658), glm::vec2(0.5625, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.578125, 0) });
	verts.push_back({ glm::vec3(0.162212, -0.980785, -0.108386), glm::vec2(0.59375, 0.0625) });
	verts.push_back({ glm::vec3(0.18024, -0.980785, -0.074658), glm::vec2(0.5625, 0.0625) });
	verts.push_back({ glm::vec3(0.831469, 0, -0.55557), glm::vec2(0.59375, 0.5) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, -0.37533), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, -0.544895), glm::vec2(0.59375, 0.4375) });
	verts.push_back({ glm::vec3(0.162212, 0.980785, -0.108386), glm::vec2(0.59375, 0.9375) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, -0.146447), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, -0.212608), glm::vec2(0.59375, 0.875) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, -0.544895), glm::vec2(0.59375, 0.4375) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, -0.353553), glm::vec2(0.5625, 0.375) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, -0.51328), glm::vec2(0.59375, 0.375) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, -0.212608), glm::vec2(0.59375, 0.875) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, -0.212607), glm::vec2(0.5625, 0.8125) });
	verts.push_back({ glm::vec3(0.46194, 0.83147, -0.308658), glm::vec2(0.59375, 0.8125) });
	verts.push_back({ glm::vec3(0.691342, -0.55557, -0.46194), glm::vec2(0.59375, 0.3125) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, -0.353553), glm::vec2(0.5625, 0.375) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, -0.31819), glm::vec2(0.5625, 0.3125) });
	verts.push_back({ glm::vec3(0.46194, 0.83147, -0.308658), glm::vec2(0.59375, 0.8125) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, -0.270598), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, -0.392847), glm::vec2(0.59375, 0.75) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, -0.392847), glm::vec2(0.59375, 0.25) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, -0.31819), glm::vec2(0.5625, 0.3125) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, -0.270598), glm::vec2(0.5625, 0.25) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, -0.37533), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(0.980785, 0, -0.19509), glm::vec2(0.53125, 0.5) });
	verts.push_back({ glm::vec3(0.96194, -0.19509, -0.191342), glm::vec2(0.53125, 0.4375) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, -0.146447), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, -0.03806), glm::vec2(0.53125, 0.9375) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, -0.074658), glm::vec2(0.53125, 0.875) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, -0.37533), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, -0.18024), glm::vec2(0.53125, 0.375) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, -0.353553), glm::vec2(0.5625, 0.375) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, -0.212607), glm::vec2(0.5625, 0.8125) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, -0.074658), glm::vec2(0.53125, 0.875) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, -0.108386), glm::vec2(0.53125, 0.8125) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, -0.31819), glm::vec2(0.5625, 0.3125) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, -0.18024), glm::vec2(0.53125, 0.375) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, -0.162212), glm::vec2(0.53125, 0.3125) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, -0.270598), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, -0.108386), glm::vec2(0.53125, 0.8125) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, -0.13795), glm::vec2(0.53125, 0.75) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, -0.31819), glm::vec2(0.5625, 0.3125) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, -0.13795), glm::vec2(0.53125, 0.25) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, -0.270598), glm::vec2(0.5625, 0.25) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, -0.31819), glm::vec2(0.5625, 0.6875) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, -0.13795), glm::vec2(0.53125, 0.75) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, -0.162212), glm::vec2(0.53125, 0.6875) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, -0.270598), glm::vec2(0.5625, 0.25) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, -0.108386), glm::vec2(0.53125, 0.1875) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, -0.212607), glm::vec2(0.5625, 0.1875) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, -0.31819), glm::vec2(0.5625, 0.6875) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, -0.18024), glm::vec2(0.53125, 0.625) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, -0.353553), glm::vec2(0.5625, 0.625) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, -0.212607), glm::vec2(0.5625, 0.1875) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, -0.074658), glm::vec2(0.53125, 0.125) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, -0.146447), glm::vec2(0.5625, 0.125) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, -0.37533), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, -0.18024), glm::vec2(0.53125, 0.625) });
	verts.push_back({ glm::vec3(0.96194, 0.19509, -0.191342), glm::vec2(0.53125, 0.5625) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, -0.146447), glm::vec2(0.5625, 0.125) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, -0.03806), glm::vec2(0.53125, 0.0625) });
	verts.push_back({ glm::vec3(0.18024, -0.980785, -0.074658), glm::vec2(0.5625, 0.0625) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, -0.37533), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.980785, 0, -0.19509), glm::vec2(0.53125, 0.5) });
	verts.push_back({ glm::vec3(0.923879, 0, -0.382683), glm::vec2(0.5625, 0.5) });
	verts.push_back({ glm::vec3(0.18024, 0.980785, -0.074658), glm::vec2(0.5625, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.546875, 1) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, -0.03806), glm::vec2(0.53125, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.546875, 0) });
	verts.push_back({ glm::vec3(0.18024, -0.980785, -0.074658), glm::vec2(0.5625, 0.0625) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, -0.03806), glm::vec2(0.53125, 0.0625) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, -0.108386), glm::vec2(0.53125, 0.1875) });
	verts.push_back({ glm::vec3(0.707106, -0.707107, 0), glm::vec2(0.5, 0.25) });
	verts.push_back({ glm::vec3(0.55557, -0.83147, 0), glm::vec2(0.5, 0.1875) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, -0.162212), glm::vec2(0.53125, 0.6875) });
	verts.push_back({ glm::vec3(0.923879, 0.382683, 0), glm::vec2(0.5, 0.625) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, -0.18024), glm::vec2(0.53125, 0.625) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, -0.074658), glm::vec2(0.53125, 0.125) });
	verts.push_back({ glm::vec3(0.55557, -0.83147, 0), glm::vec2(0.5, 0.1875) });
	verts.push_back({ glm::vec3(0.382683, -0.92388, 0), glm::vec2(0.5, 0.125) });
	verts.push_back({ glm::vec3(0.96194, 0.19509, -0.191342), glm::vec2(0.53125, 0.5625) });
	verts.push_back({ glm::vec3(0.923879, 0.382683, 0), glm::vec2(0.5, 0.625) });
	verts.push_back({ glm::vec3(0.980785, 0.19509, 0), glm::vec2(0.5, 0.5625) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, -0.03806), glm::vec2(0.53125, 0.0625) });
	verts.push_back({ glm::vec3(0.382683, -0.92388, 0), glm::vec2(0.5, 0.125) });
	verts.push_back({ glm::vec3(0.19509, -0.980785, 0), glm::vec2(0.5, 0.0625) });
	verts.push_back({ glm::vec3(0.96194, 0.19509, -0.191342), glm::vec2(0.53125, 0.5625) });
	verts.push_back({ glm::vec3(0.999999, 0, 0), glm::vec2(0.5, 0.5) });
	verts.push_back({ glm::vec3(0.980785, 0, -0.19509), glm::vec2(0.53125, 0.5) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, -0.03806), glm::vec2(0.53125, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.515625, 1) });
	verts.push_back({ glm::vec3(0.19509, 0.980785, 0), glm::vec2(0.5, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.515625, 0) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, -0.03806), glm::vec2(0.53125, 0.0625) });
	verts.push_back({ glm::vec3(0.19509, -0.980785, 0), glm::vec2(0.5, 0.0625) });
	verts.push_back({ glm::vec3(0.980785, 0, -0.19509), glm::vec2(0.53125, 0.5) });
	verts.push_back({ glm::vec3(0.980785, -0.19509, 0), glm::vec2(0.5, 0.4375) });
	verts.push_back({ glm::vec3(0.96194, -0.19509, -0.191342), glm::vec2(0.53125, 0.4375) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, -0.03806), glm::vec2(0.53125, 0.9375) });
	verts.push_back({ glm::vec3(0.382683, 0.92388, 0), glm::vec2(0.5, 0.875) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, -0.074658), glm::vec2(0.53125, 0.875) });
	verts.push_back({ glm::vec3(0.96194, -0.19509, -0.191342), glm::vec2(0.53125, 0.4375) });
	verts.push_back({ glm::vec3(0.923879, -0.382683, 0), glm::vec2(0.5, 0.375) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, -0.18024), glm::vec2(0.53125, 0.375) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, -0.074658), glm::vec2(0.53125, 0.875) });
	verts.push_back({ glm::vec3(0.55557, 0.83147, 0), glm::vec2(0.5, 0.8125) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, -0.108386), glm::vec2(0.53125, 0.8125) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, -0.162212), glm::vec2(0.53125, 0.3125) });
	verts.push_back({ glm::vec3(0.923879, -0.382683, 0), glm::vec2(0.5, 0.375) });
	verts.push_back({ glm::vec3(0.831469, -0.55557, 0), glm::vec2(0.5, 0.3125) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, -0.108386), glm::vec2(0.53125, 0.8125) });
	verts.push_back({ glm::vec3(0.707106, 0.707107, 0), glm::vec2(0.5, 0.75) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, -0.13795), glm::vec2(0.53125, 0.75) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, -0.13795), glm::vec2(0.53125, 0.25) });
	verts.push_back({ glm::vec3(0.831469, -0.55557, 0), glm::vec2(0.5, 0.3125) });
	verts.push_back({ glm::vec3(0.707106, -0.707107, 0), glm::vec2(0.5, 0.25) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, -0.13795), glm::vec2(0.53125, 0.75) });
	verts.push_back({ glm::vec3(0.831469, 0.55557, 0), glm::vec2(0.5, 0.6875) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, -0.162212), glm::vec2(0.53125, 0.6875) });
	verts.push_back({ glm::vec3(0.382683, 0.92388, 0), glm::vec2(0.5, 0.875) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, 0.03806), glm::vec2(0.46875, 0.9375) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, 0.074658), glm::vec2(0.46875, 0.875) });
	verts.push_back({ glm::vec3(0.980785, -0.19509, 0), glm::vec2(0.5, 0.4375) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, 0.18024), glm::vec2(0.46875, 0.375) });
	verts.push_back({ glm::vec3(0.923879, -0.382683, 0), glm::vec2(0.5, 0.375) });
	verts.push_back({ glm::vec3(0.55557, 0.83147, 0), glm::vec2(0.5, 0.8125) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, 0.074658), glm::vec2(0.46875, 0.875) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, 0.108386), glm::vec2(0.46875, 0.8125) });
	verts.push_back({ glm::vec3(0.831469, -0.55557, 0), glm::vec2(0.5, 0.3125) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, 0.18024), glm::vec2(0.46875, 0.375) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, 0.162212), glm::vec2(0.46875, 0.3125) });
	verts.push_back({ glm::vec3(0.707106, 0.707107, 0), glm::vec2(0.5, 0.75) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, 0.108386), glm::vec2(0.46875, 0.8125) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, 0.13795), glm::vec2(0.46875, 0.75) });
	verts.push_back({ glm::vec3(0.831469, -0.55557, 0), glm::vec2(0.5, 0.3125) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, 0.13795), glm::vec2(0.46875, 0.25) });
	verts.push_back({ glm::vec3(0.707106, -0.707107, 0), glm::vec2(0.5, 0.25) });
	verts.push_back({ glm::vec3(0.831469, 0.55557, 0), glm::vec2(0.5, 0.6875) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, 0.13795), glm::vec2(0.46875, 0.75) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, 0.162212), glm::vec2(0.46875, 0.6875) });
	verts.push_back({ glm::vec3(0.707106, -0.707107, 0), glm::vec2(0.5, 0.25) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, 0.108386), glm::vec2(0.46875, 0.1875) });
	verts.push_back({ glm::vec3(0.55557, -0.83147, 0), glm::vec2(0.5, 0.1875) });
	verts.push_back({ glm::vec3(0.831469, 0.55557, 0), glm::vec2(0.5, 0.6875) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, 0.18024), glm::vec2(0.46875, 0.625) });
	verts.push_back({ glm::vec3(0.923879, 0.382683, 0), glm::vec2(0.5, 0.625) });
	verts.push_back({ glm::vec3(0.55557, -0.83147, 0), glm::vec2(0.5, 0.1875) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, 0.074658), glm::vec2(0.46875, 0.125) });
	verts.push_back({ glm::vec3(0.382683, -0.92388, 0), glm::vec2(0.5, 0.125) });
	verts.push_back({ glm::vec3(0.980785, 0.19509, 0), glm::vec2(0.5, 0.5625) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, 0.18024), glm::vec2(0.46875, 0.625) });
	verts.push_back({ glm::vec3(0.961939, 0.19509, 0.191342), glm::vec2(0.46875, 0.5625) });
	verts.push_back({ glm::vec3(0.382683, -0.92388, 0), glm::vec2(0.5, 0.125) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, 0.03806), glm::vec2(0.46875, 0.0625) });
	verts.push_back({ glm::vec3(0.19509, -0.980785, 0), glm::vec2(0.5, 0.0625) });
	verts.push_back({ glm::vec3(0.999999, 0, 0), glm::vec2(0.5, 0.5) });
	verts.push_back({ glm::vec3(0.961939, 0.19509, 0.191342), glm::vec2(0.46875, 0.5625) });
	verts.push_back({ glm::vec3(0.980785, 0, 0.19509), glm::vec2(0.46875, 0.5) });
	verts.push_back({ glm::vec3(0.19509, 0.980785, 0), glm::vec2(0.5, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.484375, 1) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, 0.03806), glm::vec2(0.46875, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.484375, 0) });
	verts.push_back({ glm::vec3(0.19509, -0.980785, 0), glm::vec2(0.5, 0.0625) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, 0.03806), glm::vec2(0.46875, 0.0625) });
	verts.push_back({ glm::vec3(0.999999, 0, 0), glm::vec2(0.5, 0.5) });
	verts.push_back({ glm::vec3(0.961939, -0.19509, 0.191342), glm::vec2(0.46875, 0.4375) });
	verts.push_back({ glm::vec3(0.980785, -0.19509, 0), glm::vec2(0.5, 0.4375) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, 0.162212), glm::vec2(0.46875, 0.6875) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, 0.353553), glm::vec2(0.4375, 0.625) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, 0.18024), glm::vec2(0.46875, 0.625) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, 0.074658), glm::vec2(0.46875, 0.125) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, 0.212608), glm::vec2(0.4375, 0.1875) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, 0.146447), glm::vec2(0.4375, 0.125) });
	verts.push_back({ glm::vec3(0.961939, 0.19509, 0.191342), glm::vec2(0.46875, 0.5625) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, 0.353553), glm::vec2(0.4375, 0.625) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, 0.37533), glm::vec2(0.4375, 0.5625) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, 0.03806), glm::vec2(0.46875, 0.0625) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, 0.146447), glm::vec2(0.4375, 0.125) });
	verts.push_back({ glm::vec3(0.180239, -0.980785, 0.074658), glm::vec2(0.4375, 0.0625) });
	verts.push_back({ glm::vec3(0.980785, 0, 0.19509), glm::vec2(0.46875, 0.5) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, 0.37533), glm::vec2(0.4375, 0.5625) });
	verts.push_back({ glm::vec3(0.923879, 0, 0.382683), glm::vec2(0.4375, 0.5) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, 0.03806), glm::vec2(0.46875, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.453125, 1) });
	verts.push_back({ glm::vec3(0.180239, 0.980785, 0.074658), glm::vec2(0.4375, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.453125, 0) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, 0.03806), glm::vec2(0.46875, 0.0625) });
	verts.push_back({ glm::vec3(0.180239, -0.980785, 0.074658), glm::vec2(0.4375, 0.0625) });
	verts.push_back({ glm::vec3(0.980785, 0, 0.19509), glm::vec2(0.46875, 0.5) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, 0.37533), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(0.961939, -0.19509, 0.191342), glm::vec2(0.46875, 0.4375) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, 0.03806), glm::vec2(0.46875, 0.9375) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, 0.146447), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, 0.074658), glm::vec2(0.46875, 0.875) });
	verts.push_back({ glm::vec3(0.961939, -0.19509, 0.191342), glm::vec2(0.46875, 0.4375) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, 0.353553), glm::vec2(0.4375, 0.375) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, 0.18024), glm::vec2(0.46875, 0.375) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, 0.074658), glm::vec2(0.46875, 0.875) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, 0.212608), glm::vec2(0.4375, 0.8125) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, 0.108386), glm::vec2(0.46875, 0.8125) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, 0.162212), glm::vec2(0.46875, 0.3125) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, 0.353553), glm::vec2(0.4375, 0.375) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, 0.31819), glm::vec2(0.4375, 0.3125) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, 0.108386), glm::vec2(0.46875, 0.8125) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, 0.270598), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, 0.13795), glm::vec2(0.46875, 0.75) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, 0.13795), glm::vec2(0.46875, 0.25) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, 0.31819), glm::vec2(0.4375, 0.3125) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, 0.270598), glm::vec2(0.4375, 0.25) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, 0.13795), glm::vec2(0.46875, 0.75) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, 0.31819), glm::vec2(0.4375, 0.6875) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, 0.162212), glm::vec2(0.46875, 0.6875) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, 0.108386), glm::vec2(0.46875, 0.1875) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, 0.270598), glm::vec2(0.4375, 0.25) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, 0.212608), glm::vec2(0.4375, 0.1875) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, 0.353553), glm::vec2(0.4375, 0.375) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, 0.544895), glm::vec2(0.40625, 0.4375) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, 0.51328), glm::vec2(0.40625, 0.375) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, 0.146447), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(0.461939, 0.83147, 0.308658), glm::vec2(0.40625, 0.8125) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, 0.212608), glm::vec2(0.4375, 0.8125) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, 0.31819), glm::vec2(0.4375, 0.3125) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, 0.51328), glm::vec2(0.40625, 0.375) });
	verts.push_back({ glm::vec3(0.691341, -0.55557, 0.46194), glm::vec2(0.40625, 0.3125) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, 0.212608), glm::vec2(0.4375, 0.8125) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, 0.392847), glm::vec2(0.40625, 0.75) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, 0.270598), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, 0.31819), glm::vec2(0.4375, 0.3125) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, 0.392847), glm::vec2(0.40625, 0.25) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, 0.270598), glm::vec2(0.4375, 0.25) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, 0.31819), glm::vec2(0.4375, 0.6875) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, 0.392847), glm::vec2(0.40625, 0.75) });
	verts.push_back({ glm::vec3(0.691341, 0.55557, 0.46194), glm::vec2(0.40625, 0.6875) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, 0.212608), glm::vec2(0.4375, 0.1875) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, 0.392847), glm::vec2(0.40625, 0.25) });
	verts.push_back({ glm::vec3(0.461939, -0.83147, 0.308658), glm::vec2(0.40625, 0.1875) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, 0.31819), glm::vec2(0.4375, 0.6875) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, 0.51328), glm::vec2(0.40625, 0.625) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, 0.353553), glm::vec2(0.4375, 0.625) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, 0.146447), glm::vec2(0.4375, 0.125) });
	verts.push_back({ glm::vec3(0.461939, -0.83147, 0.308658), glm::vec2(0.40625, 0.1875) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, 0.212608), glm::vec2(0.40625, 0.125) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, 0.353553), glm::vec2(0.4375, 0.625) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, 0.544895), glm::vec2(0.40625, 0.5625) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, 0.37533), glm::vec2(0.4375, 0.5625) });
	verts.push_back({ glm::vec3(0.180239, -0.980785, 0.074658), glm::vec2(0.4375, 0.0625) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, 0.212608), glm::vec2(0.40625, 0.125) });
	verts.push_back({ glm::vec3(0.162211, -0.980785, 0.108386), glm::vec2(0.40625, 0.0625) });
	verts.push_back({ glm::vec3(0.923879, 0, 0.382683), glm::vec2(0.4375, 0.5) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, 0.544895), glm::vec2(0.40625, 0.5625) });
	verts.push_back({ glm::vec3(0.831469, 0, 0.55557), glm::vec2(0.40625, 0.5) });
	verts.push_back({ glm::vec3(0.180239, 0.980785, 0.074658), glm::vec2(0.4375, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.421875, 1) });
	verts.push_back({ glm::vec3(0.162211, 0.980785, 0.108386), glm::vec2(0.40625, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.421875, 0) });
	verts.push_back({ glm::vec3(0.180239, -0.980785, 0.074658), glm::vec2(0.4375, 0.0625) });
	verts.push_back({ glm::vec3(0.162211, -0.980785, 0.108386), glm::vec2(0.40625, 0.0625) });
	verts.push_back({ glm::vec3(0.923879, 0, 0.382683), glm::vec2(0.4375, 0.5) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, 0.544895), glm::vec2(0.40625, 0.4375) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, 0.37533), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(0.180239, 0.980785, 0.074658), glm::vec2(0.4375, 0.9375) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, 0.212608), glm::vec2(0.40625, 0.875) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, 0.146447), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(0.461939, -0.83147, 0.308658), glm::vec2(0.40625, 0.1875) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, 0.270598), glm::vec2(0.375, 0.125) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, 0.212608), glm::vec2(0.40625, 0.125) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, 0.51328), glm::vec2(0.40625, 0.625) });
	verts.push_back({ glm::vec3(0.693519, 0.19509, 0.69352), glm::vec2(0.375, 0.5625) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, 0.544895), glm::vec2(0.40625, 0.5625) });
	verts.push_back({ glm::vec3(0.162211, -0.980785, 0.108386), glm::vec2(0.40625, 0.0625) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, 0.270598), glm::vec2(0.375, 0.125) });
	verts.push_back({ glm::vec3(0.137949, -0.980785, 0.13795), glm::vec2(0.375, 0.0625) });
	verts.push_back({ glm::vec3(0.831469, 0, 0.55557), glm::vec2(0.40625, 0.5) });
	verts.push_back({ glm::vec3(0.693519, 0.19509, 0.69352), glm::vec2(0.375, 0.5625) });
	verts.push_back({ glm::vec3(0.707106, 0, 0.707107), glm::vec2(0.375, 0.5) });
	verts.push_back({ glm::vec3(0.162211, 0.980785, 0.108386), glm::vec2(0.40625, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.390625, 1) });
	verts.push_back({ glm::vec3(0.137949, 0.980785, 0.13795), glm::vec2(0.375, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.390625, 0) });
	verts.push_back({ glm::vec3(0.162211, -0.980785, 0.108386), glm::vec2(0.40625, 0.0625) });
	verts.push_back({ glm::vec3(0.137949, -0.980785, 0.13795), glm::vec2(0.375, 0.0625) });
	verts.push_back({ glm::vec3(0.831469, 0, 0.55557), glm::vec2(0.40625, 0.5) });
	verts.push_back({ glm::vec3(0.693519, -0.19509, 0.69352), glm::vec2(0.375, 0.4375) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, 0.544895), glm::vec2(0.40625, 0.4375) });
	verts.push_back({ glm::vec3(0.162211, 0.980785, 0.108386), glm::vec2(0.40625, 0.9375) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, 0.270598), glm::vec2(0.375, 0.875) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, 0.212608), glm::vec2(0.40625, 0.875) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, 0.51328), glm::vec2(0.40625, 0.375) });
	verts.push_back({ glm::vec3(0.693519, -0.19509, 0.69352), glm::vec2(0.375, 0.4375) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, 0.653281), glm::vec2(0.375, 0.375) });
	verts.push_back({ glm::vec3(0.461939, 0.83147, 0.308658), glm::vec2(0.40625, 0.8125) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, 0.270598), glm::vec2(0.375, 0.875) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, 0.392847), glm::vec2(0.375, 0.8125) });
	verts.push_back({ glm::vec3(0.691341, -0.55557, 0.46194), glm::vec2(0.40625, 0.3125) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, 0.653281), glm::vec2(0.375, 0.375) });
	verts.push_back({ glm::vec3(0.587937, -0.55557, 0.587938), glm::vec2(0.375, 0.3125) });
	verts.push_back({ glm::vec3(0.461939, 0.83147, 0.308658), glm::vec2(0.40625, 0.8125) });
	verts.push_back({ glm::vec3(0.5, 0.707107, 0.5), glm::vec2(0.375, 0.75) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, 0.392847), glm::vec2(0.40625, 0.75) });
	verts.push_back({ glm::vec3(0.691341, -0.55557, 0.46194), glm::vec2(0.40625, 0.3125) });
	verts.push_back({ glm::vec3(0.5, -0.707107, 0.5), glm::vec2(0.375, 0.25) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, 0.392847), glm::vec2(0.40625, 0.25) });
	verts.push_back({ glm::vec3(0.691341, 0.55557, 0.46194), glm::vec2(0.40625, 0.6875) });
	verts.push_back({ glm::vec3(0.5, 0.707107, 0.5), glm::vec2(0.375, 0.75) });
	verts.push_back({ glm::vec3(0.587937, 0.55557, 0.587938), glm::vec2(0.375, 0.6875) });
	verts.push_back({ glm::vec3(0.461939, -0.83147, 0.308658), glm::vec2(0.40625, 0.1875) });
	verts.push_back({ glm::vec3(0.5, -0.707107, 0.5), glm::vec2(0.375, 0.25) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, 0.392847), glm::vec2(0.375, 0.1875) });
	verts.push_back({ glm::vec3(0.691341, 0.55557, 0.46194), glm::vec2(0.40625, 0.6875) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, 0.653281), glm::vec2(0.375, 0.625) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, 0.51328), glm::vec2(0.40625, 0.625) });
	verts.push_back({ glm::vec3(0.587937, -0.55557, 0.587938), glm::vec2(0.375, 0.3125) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, 0.768178), glm::vec2(0.34375, 0.375) });
	verts.push_back({ glm::vec3(0.461939, -0.55557, 0.691342), glm::vec2(0.34375, 0.3125) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, 0.392847), glm::vec2(0.375, 0.8125) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, 0.587938), glm::vec2(0.34375, 0.75) });
	verts.push_back({ glm::vec3(0.5, 0.707107, 0.5), glm::vec2(0.375, 0.75) });
	verts.push_back({ glm::vec3(0.587937, -0.55557, 0.587938), glm::vec2(0.375, 0.3125) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, 0.587938), glm::vec2(0.34375, 0.25) });
	verts.push_back({ glm::vec3(0.5, -0.707107, 0.5), glm::vec2(0.375, 0.25) });
	verts.push_back({ glm::vec3(0.587937, 0.55557, 0.587938), glm::vec2(0.375, 0.6875) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, 0.587938), glm::vec2(0.34375, 0.75) });
	verts.push_back({ glm::vec3(0.461939, 0.55557, 0.691342), glm::vec2(0.34375, 0.6875) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, 0.392847), glm::vec2(0.375, 0.1875) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, 0.587938), glm::vec2(0.34375, 0.25) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, 0.46194), glm::vec2(0.34375, 0.1875) });
	verts.push_back({ glm::vec3(0.587937, 0.55557, 0.587938), glm::vec2(0.375, 0.6875) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, 0.768178), glm::vec2(0.34375, 0.625) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, 0.653281), glm::vec2(0.375, 0.625) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, 0.392847), glm::vec2(0.375, 0.1875) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, 0.31819), glm::vec2(0.34375, 0.125) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, 0.270598), glm::vec2(0.375, 0.125) });
	verts.push_back({ glm::vec3(0.693519, 0.19509, 0.69352), glm::vec2(0.375, 0.5625) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, 0.768178), glm::vec2(0.34375, 0.625) });
	verts.push_back({ glm::vec3(0.544894, 0.19509, 0.815493), glm::vec2(0.34375, 0.5625) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, 0.270598), glm::vec2(0.375, 0.125) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, 0.162212), glm::vec2(0.34375, 0.0625) });
	verts.push_back({ glm::vec3(0.137949, -0.980785, 0.13795), glm::vec2(0.375, 0.0625) });
	verts.push_back({ glm::vec3(0.707106, 0, 0.707107), glm::vec2(0.375, 0.5) });
	verts.push_back({ glm::vec3(0.544894, 0.19509, 0.815493), glm::vec2(0.34375, 0.5625) });
	verts.push_back({ glm::vec3(0.555569, 0, 0.831469), glm::vec2(0.34375, 0.5) });
	verts.push_back({ glm::vec3(0.137949, 0.980785, 0.13795), glm::vec2(0.375, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.359375, 1) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, 0.162212), glm::vec2(0.34375, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.359375, 0) });
	verts.push_back({ glm::vec3(0.137949, -0.980785, 0.13795), glm::vec2(0.375, 0.0625) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, 0.162212), glm::vec2(0.34375, 0.0625) });
	verts.push_back({ glm::vec3(0.707106, 0, 0.707107), glm::vec2(0.375, 0.5) });
	verts.push_back({ glm::vec3(0.544894, -0.19509, 0.815493), glm::vec2(0.34375, 0.4375) });
	verts.push_back({ glm::vec3(0.693519, -0.19509, 0.69352), glm::vec2(0.375, 0.4375) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, 0.270598), glm::vec2(0.375, 0.875) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, 0.162212), glm::vec2(0.34375, 0.9375) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, 0.31819), glm::vec2(0.34375, 0.875) });
	verts.push_back({ glm::vec3(0.693519, -0.19509, 0.69352), glm::vec2(0.375, 0.4375) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, 0.768178), glm::vec2(0.34375, 0.375) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, 0.653281), glm::vec2(0.375, 0.375) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, 0.392847), glm::vec2(0.375, 0.8125) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, 0.31819), glm::vec2(0.34375, 0.875) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, 0.46194), glm::vec2(0.34375, 0.8125) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, 0.31819), glm::vec2(0.34375, 0.125) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, 0.18024), glm::vec2(0.3125, 0.0625) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, 0.162212), glm::vec2(0.34375, 0.0625) });
	verts.push_back({ glm::vec3(0.555569, 0, 0.831469), glm::vec2(0.34375, 0.5) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, 0.906127), glm::vec2(0.3125, 0.5625) });
	verts.push_back({ glm::vec3(0.382683, 0, 0.923879), glm::vec2(0.3125, 0.5) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, 0.162212), glm::vec2(0.34375, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.328125, 1) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, 0.18024), glm::vec2(0.3125, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.328125, 0) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, 0.162212), glm::vec2(0.34375, 0.0625) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, 0.18024), glm::vec2(0.3125, 0.0625) });
	verts.push_back({ glm::vec3(0.555569, 0, 0.831469), glm::vec2(0.34375, 0.5) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, 0.906127), glm::vec2(0.3125, 0.4375) });
	verts.push_back({ glm::vec3(0.544894, -0.19509, 0.815493), glm::vec2(0.34375, 0.4375) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, 0.31819), glm::vec2(0.34375, 0.875) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, 0.18024), glm::vec2(0.3125, 0.9375) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, 0.353553), glm::vec2(0.3125, 0.875) });
	verts.push_back({ glm::vec3(0.544894, -0.19509, 0.815493), glm::vec2(0.34375, 0.4375) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, 0.853553), glm::vec2(0.3125, 0.375) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, 0.768178), glm::vec2(0.34375, 0.375) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, 0.46194), glm::vec2(0.34375, 0.8125) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, 0.353553), glm::vec2(0.3125, 0.875) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, 0.51328), glm::vec2(0.3125, 0.8125) });
	verts.push_back({ glm::vec3(0.461939, -0.55557, 0.691342), glm::vec2(0.34375, 0.3125) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, 0.853553), glm::vec2(0.3125, 0.375) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, 0.768178), glm::vec2(0.3125, 0.3125) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, 0.46194), glm::vec2(0.34375, 0.8125) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, 0.653281), glm::vec2(0.3125, 0.75) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, 0.587938), glm::vec2(0.34375, 0.75) });
	verts.push_back({ glm::vec3(0.461939, -0.55557, 0.691342), glm::vec2(0.34375, 0.3125) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, 0.653281), glm::vec2(0.3125, 0.25) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, 0.587938), glm::vec2(0.34375, 0.25) });
	verts.push_back({ glm::vec3(0.461939, 0.55557, 0.691342), glm::vec2(0.34375, 0.6875) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, 0.653281), glm::vec2(0.3125, 0.75) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, 0.768178), glm::vec2(0.3125, 0.6875) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, 0.46194), glm::vec2(0.34375, 0.1875) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, 0.653281), glm::vec2(0.3125, 0.25) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, 0.51328), glm::vec2(0.3125, 0.1875) });
	verts.push_back({ glm::vec3(0.461939, 0.55557, 0.691342), glm::vec2(0.34375, 0.6875) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, 0.853553), glm::vec2(0.3125, 0.625) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, 0.768178), glm::vec2(0.34375, 0.625) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, 0.46194), glm::vec2(0.34375, 0.1875) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, 0.353553), glm::vec2(0.3125, 0.125) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, 0.31819), glm::vec2(0.34375, 0.125) });
	verts.push_back({ glm::vec3(0.544894, 0.19509, 0.815493), glm::vec2(0.34375, 0.5625) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, 0.853553), glm::vec2(0.3125, 0.625) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, 0.906127), glm::vec2(0.3125, 0.5625) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, 0.51328), glm::vec2(0.3125, 0.8125) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, 0.69352), glm::vec2(0.28125, 0.75) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, 0.653281), glm::vec2(0.3125, 0.75) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, 0.653281), glm::vec2(0.3125, 0.25) });
	verts.push_back({ glm::vec3(0.162211, -0.55557, 0.815493), glm::vec2(0.28125, 0.3125) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, 0.69352), glm::vec2(0.28125, 0.25) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, 0.653281), glm::vec2(0.3125, 0.75) });
	verts.push_back({ glm::vec3(0.162211, 0.55557, 0.815493), glm::vec2(0.28125, 0.6875) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, 0.768178), glm::vec2(0.3125, 0.6875) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, 0.51328), glm::vec2(0.3125, 0.1875) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, 0.69352), glm::vec2(0.28125, 0.25) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, 0.544895), glm::vec2(0.28125, 0.1875) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, 0.768178), glm::vec2(0.3125, 0.6875) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, 0.906127), glm::vec2(0.28125, 0.625) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, 0.853553), glm::vec2(0.3125, 0.625) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, 0.51328), glm::vec2(0.3125, 0.1875) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, 0.37533), glm::vec2(0.28125, 0.125) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, 0.353553), glm::vec2(0.3125, 0.125) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, 0.906127), glm::vec2(0.3125, 0.5625) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, 0.906127), glm::vec2(0.28125, 0.625) });
	verts.push_back({ glm::vec3(0.191341, 0.19509, 0.961939), glm::vec2(0.28125, 0.5625) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, 0.18024), glm::vec2(0.3125, 0.0625) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, 0.37533), glm::vec2(0.28125, 0.125) });
	verts.push_back({ glm::vec3(0.03806, -0.980785, 0.191342), glm::vec2(0.28125, 0.0625) });
	verts.push_back({ glm::vec3(0.382683, 0, 0.923879), glm::vec2(0.3125, 0.5) });
	verts.push_back({ glm::vec3(0.191341, 0.19509, 0.961939), glm::vec2(0.28125, 0.5625) });
	verts.push_back({ glm::vec3(0.19509, 0, 0.980785), glm::vec2(0.28125, 0.5) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, 0.18024), glm::vec2(0.3125, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.296875, 1) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, 0.191342), glm::vec2(0.28125, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.296875, 0) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, 0.18024), glm::vec2(0.3125, 0.0625) });
	verts.push_back({ glm::vec3(0.03806, -0.980785, 0.191342), glm::vec2(0.28125, 0.0625) });
	verts.push_back({ glm::vec3(0.382683, 0, 0.923879), glm::vec2(0.3125, 0.5) });
	verts.push_back({ glm::vec3(0.191341, -0.19509, 0.961939), glm::vec2(0.28125, 0.4375) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, 0.906127), glm::vec2(0.3125, 0.4375) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, 0.18024), glm::vec2(0.3125, 0.9375) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, 0.37533), glm::vec2(0.28125, 0.875) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, 0.353553), glm::vec2(0.3125, 0.875) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, 0.906127), glm::vec2(0.3125, 0.4375) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, 0.906127), glm::vec2(0.28125, 0.375) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, 0.853553), glm::vec2(0.3125, 0.375) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, 0.51328), glm::vec2(0.3125, 0.8125) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, 0.37533), glm::vec2(0.28125, 0.875) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, 0.544895), glm::vec2(0.28125, 0.8125) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, 0.768178), glm::vec2(0.3125, 0.3125) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, 0.906127), glm::vec2(0.28125, 0.375) });
	verts.push_back({ glm::vec3(0.162211, -0.55557, 0.815493), glm::vec2(0.28125, 0.3125) });
	verts.push_back({ glm::vec3(0.19509, 0, 0.980785), glm::vec2(0.28125, 0.5) });
	verts.push_back({ glm::vec3(0, 0.19509, 0.980785), glm::vec2(0.25, 0.5625) });
	verts.push_back({ glm::vec3(-1E-06, 0, 0.999999), glm::vec2(0.25, 0.5) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, 0.191342), glm::vec2(0.28125, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.265625, 1) });
	verts.push_back({ glm::vec3(0, 0.980785, 0.19509), glm::vec2(0.25, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.265625, 0) });
	verts.push_back({ glm::vec3(0.03806, -0.980785, 0.191342), glm::vec2(0.28125, 0.0625) });
	verts.push_back({ glm::vec3(0, -0.980785, 0.19509), glm::vec2(0.25, 0.0625) });
	verts.push_back({ glm::vec3(0.19509, 0, 0.980785), glm::vec2(0.28125, 0.5) });
	verts.push_back({ glm::vec3(0, -0.19509, 0.980785), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(0.191341, -0.19509, 0.961939), glm::vec2(0.28125, 0.4375) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, 0.191342), glm::vec2(0.28125, 0.9375) });
	verts.push_back({ glm::vec3(0, 0.92388, 0.382683), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, 0.37533), glm::vec2(0.28125, 0.875) });
	verts.push_back({ glm::vec3(0.191341, -0.19509, 0.961939), glm::vec2(0.28125, 0.4375) });
	verts.push_back({ glm::vec3(0, -0.382683, 0.923879), glm::vec2(0.25, 0.375) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, 0.906127), glm::vec2(0.28125, 0.375) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, 0.544895), glm::vec2(0.28125, 0.8125) });
	verts.push_back({ glm::vec3(0, 0.92388, 0.382683), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(0, 0.83147, 0.55557), glm::vec2(0.25, 0.8125) });
	verts.push_back({ glm::vec3(0.162211, -0.55557, 0.815493), glm::vec2(0.28125, 0.3125) });
	verts.push_back({ glm::vec3(0, -0.382683, 0.923879), glm::vec2(0.25, 0.375) });
	verts.push_back({ glm::vec3(0, -0.55557, 0.831469), glm::vec2(0.25, 0.3125) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, 0.544895), glm::vec2(0.28125, 0.8125) });
	verts.push_back({ glm::vec3(0, 0.707107, 0.707107), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, 0.69352), glm::vec2(0.28125, 0.75) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, 0.69352), glm::vec2(0.28125, 0.25) });
	verts.push_back({ glm::vec3(0, -0.55557, 0.831469), glm::vec2(0.25, 0.3125) });
	verts.push_back({ glm::vec3(0, -0.707107, 0.707107), glm::vec2(0.25, 0.25) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, 0.69352), glm::vec2(0.28125, 0.75) });
	verts.push_back({ glm::vec3(0, 0.55557, 0.831469), glm::vec2(0.25, 0.6875) });
	verts.push_back({ glm::vec3(0.162211, 0.55557, 0.815493), glm::vec2(0.28125, 0.6875) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, 0.544895), glm::vec2(0.28125, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.707107, 0.707107), glm::vec2(0.25, 0.25) });
	verts.push_back({ glm::vec3(0, -0.83147, 0.55557), glm::vec2(0.25, 0.1875) });
	verts.push_back({ glm::vec3(0.162211, 0.55557, 0.815493), glm::vec2(0.28125, 0.6875) });
	verts.push_back({ glm::vec3(0, 0.382683, 0.923879), glm::vec2(0.25, 0.625) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, 0.906127), glm::vec2(0.28125, 0.625) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, 0.544895), glm::vec2(0.28125, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.92388, 0.382683), glm::vec2(0.25, 0.125) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, 0.37533), glm::vec2(0.28125, 0.125) });
	verts.push_back({ glm::vec3(0.191341, 0.19509, 0.961939), glm::vec2(0.28125, 0.5625) });
	verts.push_back({ glm::vec3(0, 0.382683, 0.923879), glm::vec2(0.25, 0.625) });
	verts.push_back({ glm::vec3(0, 0.19509, 0.980785), glm::vec2(0.25, 0.5625) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, 0.37533), glm::vec2(0.28125, 0.125) });
	verts.push_back({ glm::vec3(0, -0.980785, 0.19509), glm::vec2(0.25, 0.0625) });
	verts.push_back({ glm::vec3(0.03806, -0.980785, 0.191342), glm::vec2(0.28125, 0.0625) });
	verts.push_back({ glm::vec3(0, -0.55557, 0.831469), glm::vec2(0.25, 0.3125) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, 0.69352), glm::vec2(0.21875, 0.25) });
	verts.push_back({ glm::vec3(0, -0.707107, 0.707107), glm::vec2(0.25, 0.25) });
	verts.push_back({ glm::vec3(0, 0.55557, 0.831469), glm::vec2(0.25, 0.6875) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, 0.69352), glm::vec2(0.21875, 0.75) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, 0.815493), glm::vec2(0.21875, 0.6875) });
	verts.push_back({ glm::vec3(0, -0.83147, 0.55557), glm::vec2(0.25, 0.1875) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, 0.69352), glm::vec2(0.21875, 0.25) });
	verts.push_back({ glm::vec3(-0.108387, -0.83147, 0.544895), glm::vec2(0.21875, 0.1875) });
	verts.push_back({ glm::vec3(0, 0.55557, 0.831469), glm::vec2(0.25, 0.6875) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, 0.906127), glm::vec2(0.21875, 0.625) });
	verts.push_back({ glm::vec3(0, 0.382683, 0.923879), glm::vec2(0.25, 0.625) });
	verts.push_back({ glm::vec3(0, -0.83147, 0.55557), glm::vec2(0.25, 0.1875) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, 0.37533), glm::vec2(0.21875, 0.125) });
	verts.push_back({ glm::vec3(0, -0.92388, 0.382683), glm::vec2(0.25, 0.125) });
	verts.push_back({ glm::vec3(0, 0.19509, 0.980785), glm::vec2(0.25, 0.5625) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, 0.906127), glm::vec2(0.21875, 0.625) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, 0.961939), glm::vec2(0.21875, 0.5625) });
	verts.push_back({ glm::vec3(0, -0.92388, 0.382683), glm::vec2(0.25, 0.125) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, 0.191342), glm::vec2(0.21875, 0.0625) });
	verts.push_back({ glm::vec3(0, -0.980785, 0.19509), glm::vec2(0.25, 0.0625) });
	verts.push_back({ glm::vec3(-1E-06, 0, 0.999999), glm::vec2(0.25, 0.5) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, 0.961939), glm::vec2(0.21875, 0.5625) });
	verts.push_back({ glm::vec3(-0.195091, 0, 0.980785), glm::vec2(0.21875, 0.5) });
	verts.push_back({ glm::vec3(0, 0.980785, 0.19509), glm::vec2(0.25, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.234375, 1) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, 0.191342), glm::vec2(0.21875, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.234375, 0) });
	verts.push_back({ glm::vec3(0, -0.980785, 0.19509), glm::vec2(0.25, 0.0625) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, 0.191342), glm::vec2(0.21875, 0.0625) });
	verts.push_back({ glm::vec3(-1E-06, 0, 0.999999), glm::vec2(0.25, 0.5) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, 0.961939), glm::vec2(0.21875, 0.4375) });
	verts.push_back({ glm::vec3(0, -0.19509, 0.980785), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(0, 0.92388, 0.382683), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, 0.191342), glm::vec2(0.21875, 0.9375) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, 0.37533), glm::vec2(0.21875, 0.875) });
	verts.push_back({ glm::vec3(0, -0.19509, 0.980785), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, 0.906127), glm::vec2(0.21875, 0.375) });
	verts.push_back({ glm::vec3(0, -0.382683, 0.923879), glm::vec2(0.25, 0.375) });
	verts.push_back({ glm::vec3(0, 0.92388, 0.382683), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(-0.108387, 0.83147, 0.544895), glm::vec2(0.21875, 0.8125) });
	verts.push_back({ glm::vec3(0, 0.83147, 0.55557), glm::vec2(0.25, 0.8125) });
	verts.push_back({ glm::vec3(0, -0.55557, 0.831469), glm::vec2(0.25, 0.3125) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, 0.906127), glm::vec2(0.21875, 0.375) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, 0.815493), glm::vec2(0.21875, 0.3125) });
	verts.push_back({ glm::vec3(0, 0.83147, 0.55557), glm::vec2(0.25, 0.8125) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, 0.69352), glm::vec2(0.21875, 0.75) });
	verts.push_back({ glm::vec3(0, 0.707107, 0.707107), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, 0.191342), glm::vec2(0.21875, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.203125, 1) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, 0.18024), glm::vec2(0.1875, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.203125, 0) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, 0.191342), glm::vec2(0.21875, 0.0625) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, 0.18024), glm::vec2(0.1875, 0.0625) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, 0.961939), glm::vec2(0.21875, 0.4375) });
	verts.push_back({ glm::vec3(-0.382684, 0, 0.923879), glm::vec2(0.1875, 0.5) });
	verts.push_back({ glm::vec3(-0.375331, -0.19509, 0.906127), glm::vec2(0.1875, 0.4375) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, 0.37533), glm::vec2(0.21875, 0.875) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, 0.18024), glm::vec2(0.1875, 0.9375) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, 0.353553), glm::vec2(0.1875, 0.875) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, 0.961939), glm::vec2(0.21875, 0.4375) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, 0.853553), glm::vec2(0.1875, 0.375) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, 0.906127), glm::vec2(0.21875, 0.375) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, 0.37533), glm::vec2(0.21875, 0.875) });
	verts.push_back({ glm::vec3(-0.212608, 0.83147, 0.51328), glm::vec2(0.1875, 0.8125) });
	verts.push_back({ glm::vec3(-0.108387, 0.83147, 0.544895), glm::vec2(0.21875, 0.8125) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, 0.815493), glm::vec2(0.21875, 0.3125) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, 0.853553), glm::vec2(0.1875, 0.375) });
	verts.push_back({ glm::vec3(-0.31819, -0.55557, 0.768177), glm::vec2(0.1875, 0.3125) });
	verts.push_back({ glm::vec3(-0.108387, 0.83147, 0.544895), glm::vec2(0.21875, 0.8125) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, 0.653281), glm::vec2(0.1875, 0.75) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, 0.69352), glm::vec2(0.21875, 0.75) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, 0.815493), glm::vec2(0.21875, 0.3125) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, 0.653281), glm::vec2(0.1875, 0.25) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, 0.69352), glm::vec2(0.21875, 0.25) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, 0.815493), glm::vec2(0.21875, 0.6875) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, 0.653281), glm::vec2(0.1875, 0.75) });
	verts.push_back({ glm::vec3(-0.31819, 0.55557, 0.768177), glm::vec2(0.1875, 0.6875) });
	verts.push_back({ glm::vec3(-0.108387, -0.83147, 0.544895), glm::vec2(0.21875, 0.1875) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, 0.653281), glm::vec2(0.1875, 0.25) });
	verts.push_back({ glm::vec3(-0.212608, -0.83147, 0.51328), glm::vec2(0.1875, 0.1875) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, 0.815493), glm::vec2(0.21875, 0.6875) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, 0.853553), glm::vec2(0.1875, 0.625) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, 0.906127), glm::vec2(0.21875, 0.625) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, 0.37533), glm::vec2(0.21875, 0.125) });
	verts.push_back({ glm::vec3(-0.212608, -0.83147, 0.51328), glm::vec2(0.1875, 0.1875) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, 0.353553), glm::vec2(0.1875, 0.125) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, 0.961939), glm::vec2(0.21875, 0.5625) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, 0.853553), glm::vec2(0.1875, 0.625) });
	verts.push_back({ glm::vec3(-0.375331, 0.19509, 0.906127), glm::vec2(0.1875, 0.5625) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, 0.37533), glm::vec2(0.21875, 0.125) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, 0.18024), glm::vec2(0.1875, 0.0625) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, 0.191342), glm::vec2(0.21875, 0.0625) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, 0.961939), glm::vec2(0.21875, 0.5625) });
	verts.push_back({ glm::vec3(-0.382684, 0, 0.923879), glm::vec2(0.1875, 0.5) });
	verts.push_back({ glm::vec3(-0.195091, 0, 0.980785), glm::vec2(0.21875, 0.5) });
	verts.push_back({ glm::vec3(-0.31819, 0.55557, 0.768177), glm::vec2(0.1875, 0.6875) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, 0.587938), glm::vec2(0.15625, 0.75) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, 0.691341), glm::vec2(0.15625, 0.6875) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, 0.653281), glm::vec2(0.1875, 0.25) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, 0.461939), glm::vec2(0.15625, 0.1875) });
	verts.push_back({ glm::vec3(-0.212608, -0.83147, 0.51328), glm::vec2(0.1875, 0.1875) });
	verts.push_back({ glm::vec3(-0.31819, 0.55557, 0.768177), glm::vec2(0.1875, 0.6875) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, 0.768178), glm::vec2(0.15625, 0.625) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, 0.853553), glm::vec2(0.1875, 0.625) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, 0.353553), glm::vec2(0.1875, 0.125) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, 0.461939), glm::vec2(0.15625, 0.1875) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, 0.31819), glm::vec2(0.15625, 0.125) });
	verts.push_back({ glm::vec3(-0.375331, 0.19509, 0.906127), glm::vec2(0.1875, 0.5625) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, 0.768178), glm::vec2(0.15625, 0.625) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, 0.815493), glm::vec2(0.15625, 0.5625) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, 0.18024), glm::vec2(0.1875, 0.0625) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, 0.31819), glm::vec2(0.15625, 0.125) });
	verts.push_back({ glm::vec3(-0.108387, -0.980785, 0.162212), glm::vec2(0.15625, 0.0625) });
	verts.push_back({ glm::vec3(-0.375331, 0.19509, 0.906127), glm::vec2(0.1875, 0.5625) });
	verts.push_back({ glm::vec3(-0.55557, 0, 0.831469), glm::vec2(0.15625, 0.5) });
	verts.push_back({ glm::vec3(-0.382684, 0, 0.923879), glm::vec2(0.1875, 0.5) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, 0.18024), glm::vec2(0.1875, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.171875, 1) });
	verts.push_back({ glm::vec3(-0.108387, 0.980785, 0.162212), glm::vec2(0.15625, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.171875, 0) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, 0.18024), glm::vec2(0.1875, 0.0625) });
	verts.push_back({ glm::vec3(-0.108387, -0.980785, 0.162212), glm::vec2(0.15625, 0.0625) });
	verts.push_back({ glm::vec3(-0.382684, 0, 0.923879), glm::vec2(0.1875, 0.5) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, 0.815493), glm::vec2(0.15625, 0.4375) });
	verts.push_back({ glm::vec3(-0.375331, -0.19509, 0.906127), glm::vec2(0.1875, 0.4375) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, 0.18024), glm::vec2(0.1875, 0.9375) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, 0.31819), glm::vec2(0.15625, 0.875) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, 0.353553), glm::vec2(0.1875, 0.875) });
	verts.push_back({ glm::vec3(-0.375331, -0.19509, 0.906127), glm::vec2(0.1875, 0.4375) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, 0.768178), glm::vec2(0.15625, 0.375) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, 0.853553), glm::vec2(0.1875, 0.375) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, 0.353553), glm::vec2(0.1875, 0.875) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, 0.461939), glm::vec2(0.15625, 0.8125) });
	verts.push_back({ glm::vec3(-0.212608, 0.83147, 0.51328), glm::vec2(0.1875, 0.8125) });
	verts.push_back({ glm::vec3(-0.31819, -0.55557, 0.768177), glm::vec2(0.1875, 0.3125) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, 0.768178), glm::vec2(0.15625, 0.375) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, 0.691341), glm::vec2(0.15625, 0.3125) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, 0.653281), glm::vec2(0.1875, 0.75) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, 0.461939), glm::vec2(0.15625, 0.8125) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, 0.587938), glm::vec2(0.15625, 0.75) });
	verts.push_back({ glm::vec3(-0.31819, -0.55557, 0.768177), glm::vec2(0.1875, 0.3125) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, 0.587938), glm::vec2(0.15625, 0.25) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, 0.653281), glm::vec2(0.1875, 0.25) });
	verts.push_back({ glm::vec3(-0.55557, 0, 0.831469), glm::vec2(0.15625, 0.5) });
	verts.push_back({ glm::vec3(-0.69352, -0.19509, 0.69352), glm::vec2(0.125, 0.4375) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, 0.815493), glm::vec2(0.15625, 0.4375) });
	verts.push_back({ glm::vec3(-0.108387, 0.980785, 0.162212), glm::vec2(0.15625, 0.9375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, 0.270598), glm::vec2(0.125, 0.875) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, 0.31819), glm::vec2(0.15625, 0.875) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, 0.815493), glm::vec2(0.15625, 0.4375) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, 0.653281), glm::vec2(0.125, 0.375) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, 0.768178), glm::vec2(0.15625, 0.375) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, 0.461939), glm::vec2(0.15625, 0.8125) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, 0.270598), glm::vec2(0.125, 0.875) });
	verts.push_back({ glm::vec3(-0.392848, 0.83147, 0.392847), glm::vec2(0.125, 0.8125) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, 0.691341), glm::vec2(0.15625, 0.3125) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, 0.653281), glm::vec2(0.125, 0.375) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, 0.587937), glm::vec2(0.125, 0.3125) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, 0.461939), glm::vec2(0.15625, 0.8125) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, 0.5), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, 0.587938), glm::vec2(0.15625, 0.75) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, 0.691341), glm::vec2(0.15625, 0.3125) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, 0.5), glm::vec2(0.125, 0.25) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, 0.587938), glm::vec2(0.15625, 0.25) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, 0.691341), glm::vec2(0.15625, 0.6875) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, 0.5), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, 0.587937), glm::vec2(0.125, 0.6875) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, 0.461939), glm::vec2(0.15625, 0.1875) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, 0.5), glm::vec2(0.125, 0.25) });
	verts.push_back({ glm::vec3(-0.392848, -0.83147, 0.392847), glm::vec2(0.125, 0.1875) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, 0.691341), glm::vec2(0.15625, 0.6875) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, 0.653281), glm::vec2(0.125, 0.625) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, 0.768178), glm::vec2(0.15625, 0.625) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, 0.461939), glm::vec2(0.15625, 0.1875) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, 0.270598), glm::vec2(0.125, 0.125) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, 0.31819), glm::vec2(0.15625, 0.125) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, 0.815493), glm::vec2(0.15625, 0.5625) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, 0.653281), glm::vec2(0.125, 0.625) });
	verts.push_back({ glm::vec3(-0.69352, 0.19509, 0.69352), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(-0.108387, -0.980785, 0.162212), glm::vec2(0.15625, 0.0625) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, 0.270598), glm::vec2(0.125, 0.125) });
	verts.push_back({ glm::vec3(-0.13795, -0.980785, 0.13795), glm::vec2(0.125, 0.0625) });
	verts.push_back({ glm::vec3(-0.55557, 0, 0.831469), glm::vec2(0.15625, 0.5) });
	verts.push_back({ glm::vec3(-0.69352, 0.19509, 0.69352), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(-0.707107, 0, 0.707106), glm::vec2(0.125, 0.5) });
	verts.push_back({ glm::vec3(-0.108387, 0.980785, 0.162212), glm::vec2(0.15625, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.140625, 1) });
	verts.push_back({ glm::vec3(-0.13795, 0.980785, 0.13795), glm::vec2(0.125, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.140625, 0) });
	verts.push_back({ glm::vec3(-0.108387, -0.980785, 0.162212), glm::vec2(0.15625, 0.0625) });
	verts.push_back({ glm::vec3(-0.13795, -0.980785, 0.13795), glm::vec2(0.125, 0.0625) });
	verts.push_back({ glm::vec3(-0.392848, -0.83147, 0.392847), glm::vec2(0.125, 0.1875) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, 0.392847), glm::vec2(0.09375, 0.25) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, 0.308658), glm::vec2(0.09375, 0.1875) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, 0.587937), glm::vec2(0.125, 0.6875) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, 0.51328), glm::vec2(0.09375, 0.625) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, 0.653281), glm::vec2(0.125, 0.625) });
	verts.push_back({ glm::vec3(-0.392848, -0.83147, 0.392847), glm::vec2(0.125, 0.1875) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, 0.212607), glm::vec2(0.09375, 0.125) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, 0.270598), glm::vec2(0.125, 0.125) });
	verts.push_back({ glm::vec3(-0.69352, 0.19509, 0.69352), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, 0.51328), glm::vec2(0.09375, 0.625) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, 0.544895), glm::vec2(0.09375, 0.5625) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, 0.270598), glm::vec2(0.125, 0.125) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, 0.108386), glm::vec2(0.09375, 0.0625) });
	verts.push_back({ glm::vec3(-0.13795, -0.980785, 0.13795), glm::vec2(0.125, 0.0625) });
	verts.push_back({ glm::vec3(-0.707107, 0, 0.707106), glm::vec2(0.125, 0.5) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, 0.544895), glm::vec2(0.09375, 0.5625) });
	verts.push_back({ glm::vec3(-0.831469, 0, 0.555569), glm::vec2(0.09375, 0.5) });
	verts.push_back({ glm::vec3(-0.13795, 0.980785, 0.13795), glm::vec2(0.125, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.109375, 1) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, 0.108386), glm::vec2(0.09375, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.109375, 0) });
	verts.push_back({ glm::vec3(-0.13795, -0.980785, 0.13795), glm::vec2(0.125, 0.0625) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, 0.108386), glm::vec2(0.09375, 0.0625) });
	verts.push_back({ glm::vec3(-0.707107, 0, 0.707106), glm::vec2(0.125, 0.5) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, 0.544895), glm::vec2(0.09375, 0.4375) });
	verts.push_back({ glm::vec3(-0.69352, -0.19509, 0.69352), glm::vec2(0.125, 0.4375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, 0.270598), glm::vec2(0.125, 0.875) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, 0.108386), glm::vec2(0.09375, 0.9375) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, 0.212607), glm::vec2(0.09375, 0.875) });
	verts.push_back({ glm::vec3(-0.69352, -0.19509, 0.69352), glm::vec2(0.125, 0.4375) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, 0.51328), glm::vec2(0.09375, 0.375) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, 0.653281), glm::vec2(0.125, 0.375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, 0.270598), glm::vec2(0.125, 0.875) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, 0.308658), glm::vec2(0.09375, 0.8125) });
	verts.push_back({ glm::vec3(-0.392848, 0.83147, 0.392847), glm::vec2(0.125, 0.8125) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, 0.587937), glm::vec2(0.125, 0.3125) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, 0.51328), glm::vec2(0.09375, 0.375) });
	verts.push_back({ glm::vec3(-0.691342, -0.55557, 0.461939), glm::vec2(0.09375, 0.3125) });
	verts.push_back({ glm::vec3(-0.392848, 0.83147, 0.392847), glm::vec2(0.125, 0.8125) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, 0.392847), glm::vec2(0.09375, 0.75) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, 0.5), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, 0.587937), glm::vec2(0.125, 0.3125) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, 0.392847), glm::vec2(0.09375, 0.25) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, 0.5), glm::vec2(0.125, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, 0.587937), glm::vec2(0.125, 0.6875) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, 0.392847), glm::vec2(0.09375, 0.75) });
	verts.push_back({ glm::vec3(-0.691342, 0.55557, 0.461939), glm::vec2(0.09375, 0.6875) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, 0.544895), glm::vec2(0.09375, 0.4375) });
	verts.push_back({ glm::vec3(-0.853554, -0.382683, 0.353553), glm::vec2(0.0625, 0.375) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, 0.51328), glm::vec2(0.09375, 0.375) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, 0.212607), glm::vec2(0.09375, 0.875) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, 0.212607), glm::vec2(0.0625, 0.8125) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, 0.308658), glm::vec2(0.09375, 0.8125) });
	verts.push_back({ glm::vec3(-0.691342, -0.55557, 0.461939), glm::vec2(0.09375, 0.3125) });
	verts.push_back({ glm::vec3(-0.853554, -0.382683, 0.353553), glm::vec2(0.0625, 0.375) });
	verts.push_back({ glm::vec3(-0.768178, -0.55557, 0.318189), glm::vec2(0.0625, 0.3125) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, 0.308658), glm::vec2(0.09375, 0.8125) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, 0.270598), glm::vec2(0.0625, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, 0.392847), glm::vec2(0.09375, 0.75) });
	verts.push_back({ glm::vec3(-0.691342, -0.55557, 0.461939), glm::vec2(0.09375, 0.3125) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, 0.270598), glm::vec2(0.0625, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, 0.392847), glm::vec2(0.09375, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, 0.392847), glm::vec2(0.09375, 0.75) });
	verts.push_back({ glm::vec3(-0.768178, 0.55557, 0.318189), glm::vec2(0.0625, 0.6875) });
	verts.push_back({ glm::vec3(-0.691342, 0.55557, 0.461939), glm::vec2(0.09375, 0.6875) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, 0.308658), glm::vec2(0.09375, 0.1875) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, 0.270598), glm::vec2(0.0625, 0.25) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, 0.212607), glm::vec2(0.0625, 0.1875) });
	verts.push_back({ glm::vec3(-0.691342, 0.55557, 0.461939), glm::vec2(0.09375, 0.6875) });
	verts.push_back({ glm::vec3(-0.853554, 0.382683, 0.353553), glm::vec2(0.0625, 0.625) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, 0.51328), glm::vec2(0.09375, 0.625) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, 0.212607), glm::vec2(0.09375, 0.125) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, 0.212607), glm::vec2(0.0625, 0.1875) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, 0.146447), glm::vec2(0.0625, 0.125) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, 0.544895), glm::vec2(0.09375, 0.5625) });
	verts.push_back({ glm::vec3(-0.853554, 0.382683, 0.353553), glm::vec2(0.0625, 0.625) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, 0.37533), glm::vec2(0.0625, 0.5625) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, 0.212607), glm::vec2(0.09375, 0.125) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, 0.074658), glm::vec2(0.0625, 0.0625) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, 0.108386), glm::vec2(0.09375, 0.0625) });
	verts.push_back({ glm::vec3(-0.831469, 0, 0.555569), glm::vec2(0.09375, 0.5) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, 0.37533), glm::vec2(0.0625, 0.5625) });
	verts.push_back({ glm::vec3(-0.923879, 0, 0.382683), glm::vec2(0.0625, 0.5) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, 0.108386), glm::vec2(0.09375, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.078125, 1) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, 0.074658), glm::vec2(0.0625, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.078125, 0) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, 0.108386), glm::vec2(0.09375, 0.0625) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, 0.074658), glm::vec2(0.0625, 0.0625) });
	verts.push_back({ glm::vec3(-0.831469, 0, 0.555569), glm::vec2(0.09375, 0.5) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, 0.37533), glm::vec2(0.0625, 0.4375) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, 0.544895), glm::vec2(0.09375, 0.4375) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, 0.108386), glm::vec2(0.09375, 0.9375) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, 0.146447), glm::vec2(0.0625, 0.875) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, 0.212607), glm::vec2(0.09375, 0.875) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, 0.212607), glm::vec2(0.0625, 0.1875) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, 0.074658), glm::vec2(0.03125, 0.125) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, 0.146447), glm::vec2(0.0625, 0.125) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, 0.37533), glm::vec2(0.0625, 0.5625) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, 0.18024), glm::vec2(0.03125, 0.625) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, 0.191341), glm::vec2(0.03125, 0.5625) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, 0.074658), glm::vec2(0.0625, 0.0625) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, 0.074658), glm::vec2(0.03125, 0.125) });
	verts.push_back({ glm::vec3(-0.191342, -0.980785, 0.03806), glm::vec2(0.03125, 0.0625) });
	verts.push_back({ glm::vec3(-0.923879, 0, 0.382683), glm::vec2(0.0625, 0.5) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, 0.191341), glm::vec2(0.03125, 0.5625) });
	verts.push_back({ glm::vec3(-0.980785, 0, 0.19509), glm::vec2(0.03125, 0.5) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, 0.074658), glm::vec2(0.0625, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.046875, 1) });
	verts.push_back({ glm::vec3(-0.191342, 0.980785, 0.03806), glm::vec2(0.03125, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.046875, 0) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, 0.074658), glm::vec2(0.0625, 0.0625) });
	verts.push_back({ glm::vec3(-0.191342, -0.980785, 0.03806), glm::vec2(0.03125, 0.0625) });
	verts.push_back({ glm::vec3(-0.923879, 0, 0.382683), glm::vec2(0.0625, 0.5) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, 0.191341), glm::vec2(0.03125, 0.4375) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, 0.37533), glm::vec2(0.0625, 0.4375) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, 0.074658), glm::vec2(0.0625, 0.9375) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, 0.074658), glm::vec2(0.03125, 0.875) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, 0.146447), glm::vec2(0.0625, 0.875) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, 0.37533), glm::vec2(0.0625, 0.4375) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, 0.18024), glm::vec2(0.03125, 0.375) });
	verts.push_back({ glm::vec3(-0.853554, -0.382683, 0.353553), glm::vec2(0.0625, 0.375) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, 0.146447), glm::vec2(0.0625, 0.875) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, 0.108386), glm::vec2(0.03125, 0.8125) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, 0.212607), glm::vec2(0.0625, 0.8125) });
	verts.push_back({ glm::vec3(-0.768178, -0.55557, 0.318189), glm::vec2(0.0625, 0.3125) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, 0.18024), glm::vec2(0.03125, 0.375) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, 0.162211), glm::vec2(0.03125, 0.3125) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, 0.212607), glm::vec2(0.0625, 0.8125) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, 0.13795), glm::vec2(0.03125, 0.75) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, 0.270598), glm::vec2(0.0625, 0.75) });
	verts.push_back({ glm::vec3(-0.768178, -0.55557, 0.318189), glm::vec2(0.0625, 0.3125) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, 0.13795), glm::vec2(0.03125, 0.25) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, 0.270598), glm::vec2(0.0625, 0.25) });
	verts.push_back({ glm::vec3(-0.768178, 0.55557, 0.318189), glm::vec2(0.0625, 0.6875) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, 0.13795), glm::vec2(0.03125, 0.75) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, 0.162211), glm::vec2(0.03125, 0.6875) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, 0.270598), glm::vec2(0.0625, 0.25) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, 0.108386), glm::vec2(0.03125, 0.1875) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, 0.212607), glm::vec2(0.0625, 0.1875) });
	verts.push_back({ glm::vec3(-0.768178, 0.55557, 0.318189), glm::vec2(0.0625, 0.6875) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, 0.18024), glm::vec2(0.03125, 0.625) });
	verts.push_back({ glm::vec3(-0.853554, 0.382683, 0.353553), glm::vec2(0.0625, 0.625) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, 0.108386), glm::vec2(0.03125, 0.8125) });
	verts.push_back({ glm::vec3(-0.382684, 0.92388, 0), glm::vec2(0, 0.875) });
	verts.push_back({ glm::vec3(-0.55557, 0.83147, 0), glm::vec2(0, 0.8125) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, 0.162211), glm::vec2(0.03125, 0.3125) });
	verts.push_back({ glm::vec3(-0.923879, -0.382683, 0), glm::vec2(0, 0.375) });
	verts.push_back({ glm::vec3(-0.831469, -0.55557, 0), glm::vec2(0, 0.3125) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, 0.108386), glm::vec2(0.03125, 0.8125) });
	verts.push_back({ glm::vec3(-0.707107, 0.707107, 0), glm::vec2(0, 0.75) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, 0.13795), glm::vec2(0.03125, 0.75) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, 0.162211), glm::vec2(0.03125, 0.3125) });
	verts.push_back({ glm::vec3(-0.707107, -0.707107, 0), glm::vec2(0, 0.25) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, 0.13795), glm::vec2(0.03125, 0.25) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, 0.162211), glm::vec2(0.03125, 0.6875) });
	verts.push_back({ glm::vec3(-0.707107, 0.707107, 0), glm::vec2(0, 0.75) });
	verts.push_back({ glm::vec3(-0.831469, 0.55557, 0), glm::vec2(0, 0.6875) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, 0.108386), glm::vec2(0.03125, 0.1875) });
	verts.push_back({ glm::vec3(-0.707107, -0.707107, 0), glm::vec2(0, 0.25) });
	verts.push_back({ glm::vec3(-0.55557, -0.83147, 0), glm::vec2(0, 0.1875) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, 0.162211), glm::vec2(0.03125, 0.6875) });
	verts.push_back({ glm::vec3(-0.923879, 0.382683, 0), glm::vec2(0, 0.625) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, 0.18024), glm::vec2(0.03125, 0.625) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, 0.108386), glm::vec2(0.03125, 0.1875) });
	verts.push_back({ glm::vec3(-0.382684, -0.92388, 0), glm::vec2(0, 0.125) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, 0.074658), glm::vec2(0.03125, 0.125) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, 0.191341), glm::vec2(0.03125, 0.5625) });
	verts.push_back({ glm::vec3(-0.923879, 0.382683, 0), glm::vec2(0, 0.625) });
	verts.push_back({ glm::vec3(-0.980785, 0.19509, 0), glm::vec2(0, 0.5625) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, 0.074658), glm::vec2(0.03125, 0.125) });
	verts.push_back({ glm::vec3(-0.195091, -0.980785, 0), glm::vec2(0, 0.0625) });
	verts.push_back({ glm::vec3(-0.191342, -0.980785, 0.03806), glm::vec2(0.03125, 0.0625) });
	verts.push_back({ glm::vec3(-0.980785, 0, 0.19509), glm::vec2(0.03125, 0.5) });
	verts.push_back({ glm::vec3(-0.980785, 0.19509, 0), glm::vec2(0, 0.5625) });
	verts.push_back({ glm::vec3(-0.999999, 0, 0), glm::vec2(0, 0.5) });
	verts.push_back({ glm::vec3(-0.191342, 0.980785, 0.03806), glm::vec2(0.03125, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.015625, 1) });
	verts.push_back({ glm::vec3(-0.195091, 0.980785, 0), glm::vec2(0, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.015625, 0) });
	verts.push_back({ glm::vec3(-0.191342, -0.980785, 0.03806), glm::vec2(0.03125, 0.0625) });
	verts.push_back({ glm::vec3(-0.195091, -0.980785, 0), glm::vec2(0, 0.0625) });
	verts.push_back({ glm::vec3(-0.980785, 0, 0.19509), glm::vec2(0.03125, 0.5) });
	verts.push_back({ glm::vec3(-0.980785, -0.19509, 0), glm::vec2(0, 0.4375) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, 0.191341), glm::vec2(0.03125, 0.4375) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, 0.074658), glm::vec2(0.03125, 0.875) });
	verts.push_back({ glm::vec3(-0.195091, 0.980785, 0), glm::vec2(0, 0.9375) });
	verts.push_back({ glm::vec3(-0.382684, 0.92388, 0), glm::vec2(0, 0.875) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, 0.191341), glm::vec2(0.03125, 0.4375) });
	verts.push_back({ glm::vec3(-0.923879, -0.382683, 0), glm::vec2(0, 0.375) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, 0.18024), glm::vec2(0.03125, 0.375) });
	verts.push_back({ glm::vec3(-0.980785, 0.19509, 0), glm::vec2(1, 0.5625) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, -0.18024), glm::vec2(0.96875, 0.625) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, -0.191342), glm::vec2(0.96875, 0.5625) });
	verts.push_back({ glm::vec3(-0.195091, -0.980785, 0), glm::vec2(1, 0.0625) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, -0.074658), glm::vec2(0.96875, 0.125) });
	verts.push_back({ glm::vec3(-0.191342, -0.980785, -0.03806), glm::vec2(0.96875, 0.0625) });
	verts.push_back({ glm::vec3(-0.999999, 0, 0), glm::vec2(1, 0.5) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, -0.191342), glm::vec2(0.96875, 0.5625) });
	verts.push_back({ glm::vec3(-0.980785, 0, -0.195091), glm::vec2(0.96875, 0.5) });
	verts.push_back({ glm::vec3(-0.195091, 0.980785, 0), glm::vec2(1, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.984375, 1) });
	verts.push_back({ glm::vec3(-0.191342, 0.980785, -0.03806), glm::vec2(0.96875, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.984375, 0) });
	verts.push_back({ glm::vec3(-0.195091, -0.980785, 0), glm::vec2(1, 0.0625) });
	verts.push_back({ glm::vec3(-0.191342, -0.980785, -0.03806), glm::vec2(0.96875, 0.0625) });
	verts.push_back({ glm::vec3(-0.999999, 0, 0), glm::vec2(1, 0.5) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, -0.191342), glm::vec2(0.96875, 0.4375) });
	verts.push_back({ glm::vec3(-0.980785, -0.19509, 0), glm::vec2(1, 0.4375) });
	verts.push_back({ glm::vec3(-0.195091, 0.980785, 0), glm::vec2(1, 0.9375) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, -0.074658), glm::vec2(0.96875, 0.875) });
	verts.push_back({ glm::vec3(-0.382684, 0.92388, 0), glm::vec2(1, 0.875) });
	verts.push_back({ glm::vec3(-0.980785, -0.19509, 0), glm::vec2(1, 0.4375) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, -0.18024), glm::vec2(0.96875, 0.375) });
	verts.push_back({ glm::vec3(-0.923879, -0.382683, 0), glm::vec2(1, 0.375) });
	verts.push_back({ glm::vec3(-0.55557, 0.83147, 0), glm::vec2(1, 0.8125) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, -0.074658), glm::vec2(0.96875, 0.875) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, -0.108386), glm::vec2(0.96875, 0.8125) });
	verts.push_back({ glm::vec3(-0.831469, -0.55557, 0), glm::vec2(1, 0.3125) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, -0.18024), glm::vec2(0.96875, 0.375) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, -0.162212), glm::vec2(0.96875, 0.3125) });
	verts.push_back({ glm::vec3(-0.55557, 0.83147, 0), glm::vec2(1, 0.8125) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, -0.13795), glm::vec2(0.96875, 0.75) });
	verts.push_back({ glm::vec3(-0.707107, 0.707107, 0), glm::vec2(1, 0.75) });
	verts.push_back({ glm::vec3(-0.831469, -0.55557, 0), glm::vec2(1, 0.3125) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, -0.13795), glm::vec2(0.96875, 0.25) });
	verts.push_back({ glm::vec3(-0.707107, -0.707107, 0), glm::vec2(1, 0.25) });
	verts.push_back({ glm::vec3(-0.831469, 0.55557, 0), glm::vec2(1, 0.6875) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, -0.13795), glm::vec2(0.96875, 0.75) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, -0.162212), glm::vec2(0.96875, 0.6875) });
	verts.push_back({ glm::vec3(-0.55557, -0.83147, 0), glm::vec2(1, 0.1875) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, -0.13795), glm::vec2(0.96875, 0.25) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, -0.108386), glm::vec2(0.96875, 0.1875) });
	verts.push_back({ glm::vec3(-0.831469, 0.55557, 0), glm::vec2(1, 0.6875) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, -0.18024), glm::vec2(0.96875, 0.625) });
	verts.push_back({ glm::vec3(-0.923879, 0.382683, 0), glm::vec2(1, 0.625) });
	verts.push_back({ glm::vec3(-0.55557, -0.83147, 0), glm::vec2(1, 0.1875) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, -0.074658), glm::vec2(0.96875, 0.125) });
	verts.push_back({ glm::vec3(-0.382684, -0.92388, 0), glm::vec2(1, 0.125) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, -0.162212), glm::vec2(0.96875, 0.3125) });
	verts.push_back({ glm::vec3(-0.853553, -0.382683, -0.353553), glm::vec2(0.9375, 0.375) });
	verts.push_back({ glm::vec3(-0.768177, -0.55557, -0.31819), glm::vec2(0.9375, 0.3125) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, -0.108386), glm::vec2(0.96875, 0.8125) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, -0.270598), glm::vec2(0.9375, 0.75) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, -0.13795), glm::vec2(0.96875, 0.75) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, -0.162212), glm::vec2(0.96875, 0.3125) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, -0.270598), glm::vec2(0.9375, 0.25) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, -0.13795), glm::vec2(0.96875, 0.25) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, -0.162212), glm::vec2(0.96875, 0.6875) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, -0.270598), glm::vec2(0.9375, 0.75) });
	verts.push_back({ glm::vec3(-0.768177, 0.55557, -0.31819), glm::vec2(0.9375, 0.6875) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, -0.108386), glm::vec2(0.96875, 0.1875) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, -0.270598), glm::vec2(0.9375, 0.25) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, -0.212607), glm::vec2(0.9375, 0.1875) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, -0.162212), glm::vec2(0.96875, 0.6875) });
	verts.push_back({ glm::vec3(-0.853553, 0.382683, -0.353553), glm::vec2(0.9375, 0.625) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, -0.18024), glm::vec2(0.96875, 0.625) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, -0.108386), glm::vec2(0.96875, 0.1875) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, -0.146447), glm::vec2(0.9375, 0.125) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, -0.074658), glm::vec2(0.96875, 0.125) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, -0.191342), glm::vec2(0.96875, 0.5625) });
	verts.push_back({ glm::vec3(-0.853553, 0.382683, -0.353553), glm::vec2(0.9375, 0.625) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, -0.37533), glm::vec2(0.9375, 0.5625) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, -0.074658), glm::vec2(0.96875, 0.125) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, -0.074658), glm::vec2(0.9375, 0.0625) });
	verts.push_back({ glm::vec3(-0.191342, -0.980785, -0.03806), glm::vec2(0.96875, 0.0625) });
	verts.push_back({ glm::vec3(-0.980785, 0, -0.195091), glm::vec2(0.96875, 0.5) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, -0.37533), glm::vec2(0.9375, 0.5625) });
	verts.push_back({ glm::vec3(-0.923878, 0, -0.382683), glm::vec2(0.9375, 0.5) });
	verts.push_back({ glm::vec3(-0.191342, 0.980785, -0.03806), glm::vec2(0.96875, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.953125, 1) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, -0.074658), glm::vec2(0.9375, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.953125, 0) });
	verts.push_back({ glm::vec3(-0.191342, -0.980785, -0.03806), glm::vec2(0.96875, 0.0625) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, -0.074658), glm::vec2(0.9375, 0.0625) });
	verts.push_back({ glm::vec3(-0.980785, 0, -0.195091), glm::vec2(0.96875, 0.5) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, -0.37533), glm::vec2(0.9375, 0.4375) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, -0.191342), glm::vec2(0.96875, 0.4375) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, -0.074658), glm::vec2(0.96875, 0.875) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, -0.074658), glm::vec2(0.9375, 0.9375) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, -0.146447), glm::vec2(0.9375, 0.875) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, -0.191342), glm::vec2(0.96875, 0.4375) });
	verts.push_back({ glm::vec3(-0.853553, -0.382683, -0.353553), glm::vec2(0.9375, 0.375) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, -0.18024), glm::vec2(0.96875, 0.375) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, -0.108386), glm::vec2(0.96875, 0.8125) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, -0.146447), glm::vec2(0.9375, 0.875) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, -0.212607), glm::vec2(0.9375, 0.8125) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, -0.146447), glm::vec2(0.9375, 0.125) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, -0.108386), glm::vec2(0.90625, 0.0625) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, -0.074658), glm::vec2(0.9375, 0.0625) });
	verts.push_back({ glm::vec3(-0.923878, 0, -0.382683), glm::vec2(0.9375, 0.5) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, -0.544895), glm::vec2(0.90625, 0.5625) });
	verts.push_back({ glm::vec3(-0.831469, 0, -0.55557), glm::vec2(0.90625, 0.5) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, -0.074658), glm::vec2(0.9375, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.921875, 1) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, -0.108386), glm::vec2(0.90625, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.921875, 0) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, -0.074658), glm::vec2(0.9375, 0.0625) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, -0.108386), glm::vec2(0.90625, 0.0625) });
	verts.push_back({ glm::vec3(-0.923878, 0, -0.382683), glm::vec2(0.9375, 0.5) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, -0.544895), glm::vec2(0.90625, 0.4375) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, -0.37533), glm::vec2(0.9375, 0.4375) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, -0.074658), glm::vec2(0.9375, 0.9375) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, -0.212607), glm::vec2(0.90625, 0.875) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, -0.146447), glm::vec2(0.9375, 0.875) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, -0.37533), glm::vec2(0.9375, 0.4375) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, -0.51328), glm::vec2(0.90625, 0.375) });
	verts.push_back({ glm::vec3(-0.853553, -0.382683, -0.353553), glm::vec2(0.9375, 0.375) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, -0.212607), glm::vec2(0.9375, 0.8125) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, -0.212607), glm::vec2(0.90625, 0.875) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, -0.308658), glm::vec2(0.90625, 0.8125) });
	verts.push_back({ glm::vec3(-0.768177, -0.55557, -0.31819), glm::vec2(0.9375, 0.3125) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, -0.51328), glm::vec2(0.90625, 0.375) });
	verts.push_back({ glm::vec3(-0.691341, -0.55557, -0.46194), glm::vec2(0.90625, 0.3125) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, -0.270598), glm::vec2(0.9375, 0.75) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, -0.308658), glm::vec2(0.90625, 0.8125) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, -0.392847), glm::vec2(0.90625, 0.75) });
	verts.push_back({ glm::vec3(-0.768177, -0.55557, -0.31819), glm::vec2(0.9375, 0.3125) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, -0.392847), glm::vec2(0.90625, 0.25) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, -0.270598), glm::vec2(0.9375, 0.25) });
	verts.push_back({ glm::vec3(-0.768177, 0.55557, -0.31819), glm::vec2(0.9375, 0.6875) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, -0.392847), glm::vec2(0.90625, 0.75) });
	verts.push_back({ glm::vec3(-0.691341, 0.55557, -0.46194), glm::vec2(0.90625, 0.6875) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, -0.212607), glm::vec2(0.9375, 0.1875) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, -0.392847), glm::vec2(0.90625, 0.25) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, -0.308658), glm::vec2(0.90625, 0.1875) });
	verts.push_back({ glm::vec3(-0.768177, 0.55557, -0.31819), glm::vec2(0.9375, 0.6875) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, -0.51328), glm::vec2(0.90625, 0.625) });
	verts.push_back({ glm::vec3(-0.853553, 0.382683, -0.353553), glm::vec2(0.9375, 0.625) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, -0.212607), glm::vec2(0.9375, 0.1875) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, -0.212607), glm::vec2(0.90625, 0.125) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, -0.146447), glm::vec2(0.9375, 0.125) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, -0.37533), glm::vec2(0.9375, 0.5625) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, -0.51328), glm::vec2(0.90625, 0.625) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, -0.544895), glm::vec2(0.90625, 0.5625) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, -0.308658), glm::vec2(0.90625, 0.8125) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, -0.5), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, -0.392847), glm::vec2(0.90625, 0.75) });
	verts.push_back({ glm::vec3(-0.691341, -0.55557, -0.46194), glm::vec2(0.90625, 0.3125) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, -0.5), glm::vec2(0.875, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, -0.392847), glm::vec2(0.90625, 0.25) });
	verts.push_back({ glm::vec3(-0.691341, 0.55557, -0.46194), glm::vec2(0.90625, 0.6875) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, -0.5), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, -0.587938), glm::vec2(0.875, 0.6875) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, -0.308658), glm::vec2(0.90625, 0.1875) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, -0.5), glm::vec2(0.875, 0.25) });
	verts.push_back({ glm::vec3(-0.392847, -0.83147, -0.392847), glm::vec2(0.875, 0.1875) });
	verts.push_back({ glm::vec3(-0.691341, 0.55557, -0.46194), glm::vec2(0.90625, 0.6875) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, -0.653281), glm::vec2(0.875, 0.625) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, -0.51328), glm::vec2(0.90625, 0.625) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, -0.212607), glm::vec2(0.90625, 0.125) });
	verts.push_back({ glm::vec3(-0.392847, -0.83147, -0.392847), glm::vec2(0.875, 0.1875) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, -0.270598), glm::vec2(0.875, 0.125) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, -0.544895), glm::vec2(0.90625, 0.5625) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, -0.653281), glm::vec2(0.875, 0.625) });
	verts.push_back({ glm::vec3(-0.693519, 0.19509, -0.69352), glm::vec2(0.875, 0.5625) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, -0.108386), glm::vec2(0.90625, 0.0625) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, -0.270598), glm::vec2(0.875, 0.125) });
	verts.push_back({ glm::vec3(-0.13795, -0.980785, -0.13795), glm::vec2(0.875, 0.0625) });
	verts.push_back({ glm::vec3(-0.831469, 0, -0.55557), glm::vec2(0.90625, 0.5) });
	verts.push_back({ glm::vec3(-0.693519, 0.19509, -0.69352), glm::vec2(0.875, 0.5625) });
	verts.push_back({ glm::vec3(-0.707106, 0, -0.707106), glm::vec2(0.875, 0.5) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, -0.108386), glm::vec2(0.90625, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.890625, 1) });
	verts.push_back({ glm::vec3(-0.13795, 0.980785, -0.13795), glm::vec2(0.875, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.890625, 0) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, -0.108386), glm::vec2(0.90625, 0.0625) });
	verts.push_back({ glm::vec3(-0.13795, -0.980785, -0.13795), glm::vec2(0.875, 0.0625) });
	verts.push_back({ glm::vec3(-0.831469, 0, -0.55557), glm::vec2(0.90625, 0.5) });
	verts.push_back({ glm::vec3(-0.693519, -0.19509, -0.69352), glm::vec2(0.875, 0.4375) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, -0.544895), glm::vec2(0.90625, 0.4375) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, -0.108386), glm::vec2(0.90625, 0.9375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, -0.270598), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, -0.212607), glm::vec2(0.90625, 0.875) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, -0.544895), glm::vec2(0.90625, 0.4375) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, -0.653281), glm::vec2(0.875, 0.375) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, -0.51328), glm::vec2(0.90625, 0.375) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, -0.212607), glm::vec2(0.90625, 0.875) });
	verts.push_back({ glm::vec3(-0.392847, 0.83147, -0.392847), glm::vec2(0.875, 0.8125) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, -0.308658), glm::vec2(0.90625, 0.8125) });
	verts.push_back({ glm::vec3(-0.691341, -0.55557, -0.46194), glm::vec2(0.90625, 0.3125) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, -0.653281), glm::vec2(0.875, 0.375) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, -0.587938), glm::vec2(0.875, 0.3125) });
	verts.push_back({ glm::vec3(-0.707106, 0, -0.707106), glm::vec2(0.875, 0.5) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, -0.815493), glm::vec2(0.84375, 0.5625) });
	verts.push_back({ glm::vec3(-0.555569, 0, -0.831469), glm::vec2(0.84375, 0.5) });
	verts.push_back({ glm::vec3(-0.13795, 0.980785, -0.13795), glm::vec2(0.875, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.859375, 1) });
	verts.push_back({ glm::vec3(-0.108386, 0.980785, -0.162212), glm::vec2(0.84375, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.859375, 0) });
	verts.push_back({ glm::vec3(-0.13795, -0.980785, -0.13795), glm::vec2(0.875, 0.0625) });
	verts.push_back({ glm::vec3(-0.108386, -0.980785, -0.162212), glm::vec2(0.84375, 0.0625) });
	verts.push_back({ glm::vec3(-0.707106, 0, -0.707106), glm::vec2(0.875, 0.5) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, -0.815493), glm::vec2(0.84375, 0.4375) });
	verts.push_back({ glm::vec3(-0.693519, -0.19509, -0.69352), glm::vec2(0.875, 0.4375) });
	verts.push_back({ glm::vec3(-0.13795, 0.980785, -0.13795), glm::vec2(0.875, 0.9375) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, -0.31819), glm::vec2(0.84375, 0.875) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, -0.270598), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-0.693519, -0.19509, -0.69352), glm::vec2(0.875, 0.4375) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, -0.768177), glm::vec2(0.84375, 0.375) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, -0.653281), glm::vec2(0.875, 0.375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, -0.270598), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, -0.461939), glm::vec2(0.84375, 0.8125) });
	verts.push_back({ glm::vec3(-0.392847, 0.83147, -0.392847), glm::vec2(0.875, 0.8125) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, -0.587938), glm::vec2(0.875, 0.3125) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, -0.768177), glm::vec2(0.84375, 0.375) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, -0.691341), glm::vec2(0.84375, 0.3125) });
	verts.push_back({ glm::vec3(-0.392847, 0.83147, -0.392847), glm::vec2(0.875, 0.8125) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, -0.587938), glm::vec2(0.84375, 0.75) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, -0.5), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, -0.587938), glm::vec2(0.875, 0.3125) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, -0.587938), glm::vec2(0.84375, 0.25) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, -0.5), glm::vec2(0.875, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, -0.587938), glm::vec2(0.875, 0.6875) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, -0.587938), glm::vec2(0.84375, 0.75) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, -0.691341), glm::vec2(0.84375, 0.6875) });
	verts.push_back({ glm::vec3(-0.392847, -0.83147, -0.392847), glm::vec2(0.875, 0.1875) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, -0.587938), glm::vec2(0.84375, 0.25) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, -0.461939), glm::vec2(0.84375, 0.1875) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, -0.587938), glm::vec2(0.875, 0.6875) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, -0.768177), glm::vec2(0.84375, 0.625) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, -0.653281), glm::vec2(0.875, 0.625) });
	verts.push_back({ glm::vec3(-0.392847, -0.83147, -0.392847), glm::vec2(0.875, 0.1875) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, -0.31819), glm::vec2(0.84375, 0.125) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, -0.270598), glm::vec2(0.875, 0.125) });
	verts.push_back({ glm::vec3(-0.693519, 0.19509, -0.69352), glm::vec2(0.875, 0.5625) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, -0.768177), glm::vec2(0.84375, 0.625) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, -0.815493), glm::vec2(0.84375, 0.5625) });
	verts.push_back({ glm::vec3(-0.13795, -0.980785, -0.13795), glm::vec2(0.875, 0.0625) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, -0.31819), glm::vec2(0.84375, 0.125) });
	verts.push_back({ glm::vec3(-0.108386, -0.980785, -0.162212), glm::vec2(0.84375, 0.0625) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, -0.691341), glm::vec2(0.84375, 0.3125) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, -0.653281), glm::vec2(0.8125, 0.25) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, -0.587938), glm::vec2(0.84375, 0.25) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, -0.691341), glm::vec2(0.84375, 0.6875) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, -0.653281), glm::vec2(0.8125, 0.75) });
	verts.push_back({ glm::vec3(-0.318189, 0.55557, -0.768177), glm::vec2(0.8125, 0.6875) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, -0.461939), glm::vec2(0.84375, 0.1875) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, -0.653281), glm::vec2(0.8125, 0.25) });
	verts.push_back({ glm::vec3(-0.212607, -0.83147, -0.513279), glm::vec2(0.8125, 0.1875) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, -0.691341), glm::vec2(0.84375, 0.6875) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, -0.853553), glm::vec2(0.8125, 0.625) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, -0.768177), glm::vec2(0.84375, 0.625) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, -0.461939), glm::vec2(0.84375, 0.1875) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, -0.353553), glm::vec2(0.8125, 0.125) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, -0.31819), glm::vec2(0.84375, 0.125) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, -0.815493), glm::vec2(0.84375, 0.5625) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, -0.853553), glm::vec2(0.8125, 0.625) });
	verts.push_back({ glm::vec3(-0.37533, 0.19509, -0.906127), glm::vec2(0.8125, 0.5625) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, -0.31819), glm::vec2(0.84375, 0.125) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, -0.18024), glm::vec2(0.8125, 0.0625) });
	verts.push_back({ glm::vec3(-0.108386, -0.980785, -0.162212), glm::vec2(0.84375, 0.0625) });
	verts.push_back({ glm::vec3(-0.555569, 0, -0.831469), glm::vec2(0.84375, 0.5) });
	verts.push_back({ glm::vec3(-0.37533, 0.19509, -0.906127), glm::vec2(0.8125, 0.5625) });
	verts.push_back({ glm::vec3(-0.382683, 0, -0.923879), glm::vec2(0.8125, 0.5) });
	verts.push_back({ glm::vec3(-0.108386, 0.980785, -0.162212), glm::vec2(0.84375, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.828125, 1) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, -0.18024), glm::vec2(0.8125, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.828125, 0) });
	verts.push_back({ glm::vec3(-0.108386, -0.980785, -0.162212), glm::vec2(0.84375, 0.0625) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, -0.18024), glm::vec2(0.8125, 0.0625) });
	verts.push_back({ glm::vec3(-0.555569, 0, -0.831469), glm::vec2(0.84375, 0.5) });
	verts.push_back({ glm::vec3(-0.37533, -0.19509, -0.906127), glm::vec2(0.8125, 0.4375) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, -0.815493), glm::vec2(0.84375, 0.4375) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, -0.31819), glm::vec2(0.84375, 0.875) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, -0.18024), glm::vec2(0.8125, 0.9375) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, -0.353553), glm::vec2(0.8125, 0.875) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, -0.815493), glm::vec2(0.84375, 0.4375) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, -0.853553), glm::vec2(0.8125, 0.375) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, -0.768177), glm::vec2(0.84375, 0.375) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, -0.461939), glm::vec2(0.84375, 0.8125) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, -0.353553), glm::vec2(0.8125, 0.875) });
	verts.push_back({ glm::vec3(-0.212607, 0.83147, -0.513279), glm::vec2(0.8125, 0.8125) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, -0.691341), glm::vec2(0.84375, 0.3125) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, -0.853553), glm::vec2(0.8125, 0.375) });
	verts.push_back({ glm::vec3(-0.318189, -0.55557, -0.768177), glm::vec2(0.8125, 0.3125) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, -0.461939), glm::vec2(0.84375, 0.8125) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, -0.653281), glm::vec2(0.8125, 0.75) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, -0.587938), glm::vec2(0.84375, 0.75) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.796875, 0) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, -0.18024), glm::vec2(0.8125, 0.0625) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, -0.191342), glm::vec2(0.78125, 0.0625) });
	verts.push_back({ glm::vec3(-0.382683, 0, -0.923879), glm::vec2(0.8125, 0.5) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, -0.961939), glm::vec2(0.78125, 0.4375) });
	verts.push_back({ glm::vec3(-0.37533, -0.19509, -0.906127), glm::vec2(0.8125, 0.4375) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, -0.18024), glm::vec2(0.8125, 0.9375) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, -0.37533), glm::vec2(0.78125, 0.875) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, -0.353553), glm::vec2(0.8125, 0.875) });
	verts.push_back({ glm::vec3(-0.37533, -0.19509, -0.906127), glm::vec2(0.8125, 0.4375) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, -0.906127), glm::vec2(0.78125, 0.375) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, -0.853553), glm::vec2(0.8125, 0.375) });
	verts.push_back({ glm::vec3(-0.212607, 0.83147, -0.513279), glm::vec2(0.8125, 0.8125) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, -0.37533), glm::vec2(0.78125, 0.875) });
	verts.push_back({ glm::vec3(-0.108386, 0.83147, -0.544895), glm::vec2(0.78125, 0.8125) });
	verts.push_back({ glm::vec3(-0.318189, -0.55557, -0.768177), glm::vec2(0.8125, 0.3125) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, -0.906127), glm::vec2(0.78125, 0.375) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, -0.815493), glm::vec2(0.78125, 0.3125) });
	verts.push_back({ glm::vec3(-0.212607, 0.83147, -0.513279), glm::vec2(0.8125, 0.8125) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, -0.69352), glm::vec2(0.78125, 0.75) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, -0.653281), glm::vec2(0.8125, 0.75) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, -0.653281), glm::vec2(0.8125, 0.25) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, -0.815493), glm::vec2(0.78125, 0.3125) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, -0.69352), glm::vec2(0.78125, 0.25) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, -0.653281), glm::vec2(0.8125, 0.75) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, -0.815493), glm::vec2(0.78125, 0.6875) });
	verts.push_back({ glm::vec3(-0.318189, 0.55557, -0.768177), glm::vec2(0.8125, 0.6875) });
	verts.push_back({ glm::vec3(-0.212607, -0.83147, -0.513279), glm::vec2(0.8125, 0.1875) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, -0.69352), glm::vec2(0.78125, 0.25) });
	verts.push_back({ glm::vec3(-0.108386, -0.83147, -0.544895), glm::vec2(0.78125, 0.1875) });
	verts.push_back({ glm::vec3(-0.318189, 0.55557, -0.768177), glm::vec2(0.8125, 0.6875) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, -0.906127), glm::vec2(0.78125, 0.625) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, -0.853553), glm::vec2(0.8125, 0.625) });
	verts.push_back({ glm::vec3(-0.212607, -0.83147, -0.513279), glm::vec2(0.8125, 0.1875) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, -0.37533), glm::vec2(0.78125, 0.125) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, -0.353553), glm::vec2(0.8125, 0.125) });
	verts.push_back({ glm::vec3(-0.37533, 0.19509, -0.906127), glm::vec2(0.8125, 0.5625) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, -0.906127), glm::vec2(0.78125, 0.625) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, -0.961939), glm::vec2(0.78125, 0.5625) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, -0.18024), glm::vec2(0.8125, 0.0625) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, -0.37533), glm::vec2(0.78125, 0.125) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, -0.191342), glm::vec2(0.78125, 0.0625) });
	verts.push_back({ glm::vec3(-0.382683, 0, -0.923879), glm::vec2(0.8125, 0.5) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, -0.961939), glm::vec2(0.78125, 0.5625) });
	verts.push_back({ glm::vec3(-0.19509, 0, -0.980784), glm::vec2(0.78125, 0.5) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, -0.18024), glm::vec2(0.8125, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.796875, 1) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, -0.191342), glm::vec2(0.78125, 0.9375) });
	verts.push_back({ glm::vec3(-0.108386, -0.83147, -0.544895), glm::vec2(0.78125, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.707107, -0.707107), glm::vec2(0.75, 0.25) });
	verts.push_back({ glm::vec3(0, -0.83147, -0.55557), glm::vec2(0.75, 0.1875) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, -0.906127), glm::vec2(0.78125, 0.625) });
	verts.push_back({ glm::vec3(0, 0.55557, -0.83147), glm::vec2(0.75, 0.6875) });
	verts.push_back({ glm::vec3(0, 0.382683, -0.923879), glm::vec2(0.75, 0.625) });
	verts.push_back({ glm::vec3(-0.108386, -0.83147, -0.544895), glm::vec2(0.78125, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.92388, -0.382683), glm::vec2(0.75, 0.125) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, -0.37533), glm::vec2(0.78125, 0.125) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, -0.961939), glm::vec2(0.78125, 0.5625) });
	verts.push_back({ glm::vec3(0, 0.382683, -0.923879), glm::vec2(0.75, 0.625) });
	verts.push_back({ glm::vec3(0, 0.19509, -0.980785), glm::vec2(0.75, 0.5625) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, -0.191342), glm::vec2(0.78125, 0.0625) });
	verts.push_back({ glm::vec3(0, -0.92388, -0.382683), glm::vec2(0.75, 0.125) });
	verts.push_back({ glm::vec3(0, -0.980785, -0.19509), glm::vec2(0.75, 0.0625) });
	verts.push_back({ glm::vec3(-0.19509, 0, -0.980784), glm::vec2(0.78125, 0.5) });
	verts.push_back({ glm::vec3(0, 0.19509, -0.980785), glm::vec2(0.75, 0.5625) });
	verts.push_back({ glm::vec3(0, 0, -1), glm::vec2(0.75, 0.5) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, -0.191342), glm::vec2(0.78125, 0.9375) });
	verts.push_back({ glm::vec3(0, 1, 0), glm::vec2(0.765625, 1) });
	verts.push_back({ glm::vec3(0, 0.980785, -0.19509), glm::vec2(0.75, 0.9375) });
	verts.push_back({ glm::vec3(0, -1, 0), glm::vec2(0.765625, 0) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, -0.191342), glm::vec2(0.78125, 0.0625) });
	verts.push_back({ glm::vec3(0, -0.980785, -0.19509), glm::vec2(0.75, 0.0625) });
	verts.push_back({ glm::vec3(-0.19509, 0, -0.980784), glm::vec2(0.78125, 0.5) });
	verts.push_back({ glm::vec3(0, -0.19509, -0.980785), glm::vec2(0.75, 0.4375) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, -0.961939), glm::vec2(0.78125, 0.4375) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, -0.191342), glm::vec2(0.78125, 0.9375) });
	verts.push_back({ glm::vec3(0, 0.92388, -0.382683), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, -0.37533), glm::vec2(0.78125, 0.875) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, -0.961939), glm::vec2(0.78125, 0.4375) });
	verts.push_back({ glm::vec3(0, -0.382683, -0.923879), glm::vec2(0.75, 0.375) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, -0.906127), glm::vec2(0.78125, 0.375) });
	verts.push_back({ glm::vec3(-0.108386, 0.83147, -0.544895), glm::vec2(0.78125, 0.8125) });
	verts.push_back({ glm::vec3(0, 0.92388, -0.382683), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(0, 0.83147, -0.55557), glm::vec2(0.75, 0.8125) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, -0.906127), glm::vec2(0.78125, 0.375) });
	verts.push_back({ glm::vec3(0, -0.55557, -0.83147), glm::vec2(0.75, 0.3125) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, -0.815493), glm::vec2(0.78125, 0.3125) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, -0.69352), glm::vec2(0.78125, 0.75) });
	verts.push_back({ glm::vec3(0, 0.83147, -0.55557), glm::vec2(0.75, 0.8125) });
	verts.push_back({ glm::vec3(0, 0.707107, -0.707107), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, -0.815493), glm::vec2(0.78125, 0.3125) });
	verts.push_back({ glm::vec3(0, -0.707107, -0.707107), glm::vec2(0.75, 0.25) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, -0.69352), glm::vec2(0.78125, 0.25) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, -0.815493), glm::vec2(0.78125, 0.6875) });
	verts.push_back({ glm::vec3(0, 0.707107, -0.707107), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(0, 0.55557, -0.83147), glm::vec2(0.75, 0.6875) });
	verts.push_back({ glm::vec3(0, 0.55557, -0.83147), glm::vec2(0.75, 0.6875) });
	verts.push_back({ glm::vec3(0, 0.707107, -0.707107), glm::vec2(0.75, 0.75) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, -0.69352), glm::vec2(0.71875, 0.75) });
	verts.push_back({ glm::vec3(0, -0.707107, -0.707107), glm::vec2(0.75, 0.25) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, -0.69352), glm::vec2(0.71875, 0.25) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, -0.544895), glm::vec2(0.71875, 0.1875) });
	verts.push_back({ glm::vec3(0, 0.382683, -0.923879), glm::vec2(0.75, 0.625) });
	verts.push_back({ glm::vec3(0, 0.55557, -0.83147), glm::vec2(0.75, 0.6875) });
	verts.push_back({ glm::vec3(0.162212, 0.55557, -0.815493), glm::vec2(0.71875, 0.6875) });
	verts.push_back({ glm::vec3(0, -0.83147, -0.55557), glm::vec2(0.75, 0.1875) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, -0.544895), glm::vec2(0.71875, 0.1875) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, -0.37533), glm::vec2(0.71875, 0.125) });
	verts.push_back({ glm::vec3(0, 0.382683, -0.923879), glm::vec2(0.75, 0.625) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, -0.906127), glm::vec2(0.71875, 0.625) });
	verts.push_back({ glm::vec3(0.191342, 0.19509, -0.96194), glm::vec2(0.71875, 0.5625) });
	verts.push_back({ glm::vec3(0, -0.92388, -0.382683), glm::vec2(0.75, 0.125) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, -0.37533), glm::vec2(0.71875, 0.125) });
	verts.push_back({ glm::vec3(0.03806, -0.980785, -0.191342), glm::vec2(0.71875, 0.0625) });
	verts.push_back({ glm::vec3(0, 0.19509, -0.980785), glm::vec2(0.75, 0.5625) });
	verts.push_back({ glm::vec3(0.191342, 0.19509, -0.96194), glm::vec2(0.71875, 0.5625) });
	verts.push_back({ glm::vec3(0.19509, 0, -0.980785), glm::vec2(0.71875, 0.5) });
	verts.push_back({ glm::vec3(0, -0.19509, -0.980785), glm::vec2(0.75, 0.4375) });
	verts.push_back({ glm::vec3(0, 0, -1), glm::vec2(0.75, 0.5) });
	verts.push_back({ glm::vec3(0.19509, 0, -0.980785), glm::vec2(0.71875, 0.5) });
	verts.push_back({ glm::vec3(0, 0.92388, -0.382683), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(0, 0.980785, -0.19509), glm::vec2(0.75, 0.9375) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, -0.191342), glm::vec2(0.71875, 0.9375) });
	verts.push_back({ glm::vec3(0, -0.19509, -0.980785), glm::vec2(0.75, 0.4375) });
	verts.push_back({ glm::vec3(0.191342, -0.19509, -0.96194), glm::vec2(0.71875, 0.4375) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, -0.906127), glm::vec2(0.71875, 0.375) });
	verts.push_back({ glm::vec3(0, 0.83147, -0.55557), glm::vec2(0.75, 0.8125) });
	verts.push_back({ glm::vec3(0, 0.92388, -0.382683), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, -0.37533), glm::vec2(0.71875, 0.875) });
	verts.push_back({ glm::vec3(0, -0.382683, -0.923879), glm::vec2(0.75, 0.375) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, -0.906127), glm::vec2(0.71875, 0.375) });
	verts.push_back({ glm::vec3(0.162212, -0.55557, -0.815493), glm::vec2(0.71875, 0.3125) });
	verts.push_back({ glm::vec3(0, 0.83147, -0.55557), glm::vec2(0.75, 0.8125) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, -0.544895), glm::vec2(0.71875, 0.8125) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, -0.69352), glm::vec2(0.71875, 0.75) });
	verts.push_back({ glm::vec3(0, -0.55557, -0.83147), glm::vec2(0.75, 0.3125) });
	verts.push_back({ glm::vec3(0.162212, -0.55557, -0.815493), glm::vec2(0.71875, 0.3125) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, -0.69352), glm::vec2(0.71875, 0.25) });
	verts.push_back({ glm::vec3(0.162212, -0.55557, -0.815493), glm::vec2(0.71875, 0.3125) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, -0.906127), glm::vec2(0.71875, 0.375) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, -0.853553), glm::vec2(0.6875, 0.375) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, -0.544895), glm::vec2(0.71875, 0.8125) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, -0.51328), glm::vec2(0.6875, 0.8125) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, -0.653281), glm::vec2(0.6875, 0.75) });
	verts.push_back({ glm::vec3(0.162212, -0.55557, -0.815493), glm::vec2(0.71875, 0.3125) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, -0.768178), glm::vec2(0.6875, 0.3125) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, -0.653281), glm::vec2(0.6875, 0.25) });
	verts.push_back({ glm::vec3(0.162212, 0.55557, -0.815493), glm::vec2(0.71875, 0.6875) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, -0.69352), glm::vec2(0.71875, 0.75) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, -0.653281), glm::vec2(0.6875, 0.75) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, -0.544895), glm::vec2(0.71875, 0.1875) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, -0.69352), glm::vec2(0.71875, 0.25) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, -0.653281), glm::vec2(0.6875, 0.25) });
	verts.push_back({ glm::vec3(0.162212, 0.55557, -0.815493), glm::vec2(0.71875, 0.6875) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, -0.768178), glm::vec2(0.6875, 0.6875) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, -0.853553), glm::vec2(0.6875, 0.625) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, -0.37533), glm::vec2(0.71875, 0.125) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, -0.544895), glm::vec2(0.71875, 0.1875) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, -0.51328), glm::vec2(0.6875, 0.1875) });
	verts.push_back({ glm::vec3(0.191342, 0.19509, -0.96194), glm::vec2(0.71875, 0.5625) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, -0.906127), glm::vec2(0.71875, 0.625) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, -0.853553), glm::vec2(0.6875, 0.625) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, -0.37533), glm::vec2(0.71875, 0.125) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, -0.353553), glm::vec2(0.6875, 0.125) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, -0.18024), glm::vec2(0.6875, 0.0625) });
	verts.push_back({ glm::vec3(0.191342, 0.19509, -0.96194), glm::vec2(0.71875, 0.5625) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, -0.906127), glm::vec2(0.6875, 0.5625) });
	verts.push_back({ glm::vec3(0.382683, 0, -0.923879), glm::vec2(0.6875, 0.5) });
	verts.push_back({ glm::vec3(0.191342, -0.19509, -0.96194), glm::vec2(0.71875, 0.4375) });
	verts.push_back({ glm::vec3(0.19509, 0, -0.980785), glm::vec2(0.71875, 0.5) });
	verts.push_back({ glm::vec3(0.382683, 0, -0.923879), glm::vec2(0.6875, 0.5) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, -0.37533), glm::vec2(0.71875, 0.875) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, -0.191342), glm::vec2(0.71875, 0.9375) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, -0.18024), glm::vec2(0.6875, 0.9375) });
	verts.push_back({ glm::vec3(0.191342, -0.19509, -0.96194), glm::vec2(0.71875, 0.4375) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, -0.906127), glm::vec2(0.6875, 0.4375) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, -0.853553), glm::vec2(0.6875, 0.375) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, -0.37533), glm::vec2(0.71875, 0.875) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, -0.353553), glm::vec2(0.6875, 0.875) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, -0.51328), glm::vec2(0.6875, 0.8125) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, -0.18024), glm::vec2(0.6875, 0.0625) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, -0.353553), glm::vec2(0.6875, 0.125) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, -0.31819), glm::vec2(0.65625, 0.125) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, -0.906127), glm::vec2(0.6875, 0.5625) });
	verts.push_back({ glm::vec3(0.544895, 0.19509, -0.815493), glm::vec2(0.65625, 0.5625) });
	verts.push_back({ glm::vec3(0.55557, 0, -0.831469), glm::vec2(0.65625, 0.5) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, -0.906127), glm::vec2(0.6875, 0.4375) });
	verts.push_back({ glm::vec3(0.382683, 0, -0.923879), glm::vec2(0.6875, 0.5) });
	verts.push_back({ glm::vec3(0.55557, 0, -0.831469), glm::vec2(0.65625, 0.5) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, -0.18024), glm::vec2(0.6875, 0.9375) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, -0.162212), glm::vec2(0.65625, 0.9375) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, -0.31819), glm::vec2(0.65625, 0.875) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, -0.906127), glm::vec2(0.6875, 0.4375) });
	verts.push_back({ glm::vec3(0.544895, -0.19509, -0.815493), glm::vec2(0.65625, 0.4375) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, -0.768178), glm::vec2(0.65625, 0.375) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, -0.353553), glm::vec2(0.6875, 0.875) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, -0.31819), glm::vec2(0.65625, 0.875) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, -0.46194), glm::vec2(0.65625, 0.8125) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, -0.768178), glm::vec2(0.6875, 0.3125) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, -0.853553), glm::vec2(0.6875, 0.375) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, -0.768178), glm::vec2(0.65625, 0.375) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, -0.653281), glm::vec2(0.6875, 0.75) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, -0.51328), glm::vec2(0.6875, 0.8125) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, -0.46194), glm::vec2(0.65625, 0.8125) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, -0.768178), glm::vec2(0.6875, 0.3125) });
	verts.push_back({ glm::vec3(0.46194, -0.55557, -0.691342), glm::vec2(0.65625, 0.3125) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, -0.587938), glm::vec2(0.65625, 0.25) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, -0.768178), glm::vec2(0.6875, 0.6875) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, -0.653281), glm::vec2(0.6875, 0.75) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, -0.587938), glm::vec2(0.65625, 0.75) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, -0.653281), glm::vec2(0.6875, 0.25) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, -0.587938), glm::vec2(0.65625, 0.25) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, -0.46194), glm::vec2(0.65625, 0.1875) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, -0.768178), glm::vec2(0.6875, 0.6875) });
	verts.push_back({ glm::vec3(0.46194, 0.55557, -0.691342), glm::vec2(0.65625, 0.6875) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, -0.768178), glm::vec2(0.65625, 0.625) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, -0.353553), glm::vec2(0.6875, 0.125) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, -0.51328), glm::vec2(0.6875, 0.1875) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, -0.46194), glm::vec2(0.65625, 0.1875) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, -0.906127), glm::vec2(0.6875, 0.5625) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, -0.853553), glm::vec2(0.6875, 0.625) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, -0.768178), glm::vec2(0.65625, 0.625) });
	verts.push_back({ glm::vec3(0.46194, -0.55557, -0.691342), glm::vec2(0.65625, 0.3125) });
	verts.push_back({ glm::vec3(0.587938, -0.55557, -0.587938), glm::vec2(0.625, 0.3125) });
	verts.push_back({ glm::vec3(0.5, -0.707107, -0.5), glm::vec2(0.625, 0.25) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, -0.587938), glm::vec2(0.65625, 0.75) });
	verts.push_back({ glm::vec3(0.5, 0.707107, -0.5), glm::vec2(0.625, 0.75) });
	verts.push_back({ glm::vec3(0.587938, 0.55557, -0.587938), glm::vec2(0.625, 0.6875) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, -0.587938), glm::vec2(0.65625, 0.25) });
	verts.push_back({ glm::vec3(0.5, -0.707107, -0.5), glm::vec2(0.625, 0.25) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, -0.392847), glm::vec2(0.625, 0.1875) });
	verts.push_back({ glm::vec3(0.46194, 0.55557, -0.691342), glm::vec2(0.65625, 0.6875) });
	verts.push_back({ glm::vec3(0.587938, 0.55557, -0.587938), glm::vec2(0.625, 0.6875) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, -0.653281), glm::vec2(0.625, 0.625) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, -0.46194), glm::vec2(0.65625, 0.1875) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, -0.392847), glm::vec2(0.625, 0.1875) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, -0.270598), glm::vec2(0.625, 0.125) });
	verts.push_back({ glm::vec3(0.544895, 0.19509, -0.815493), glm::vec2(0.65625, 0.5625) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, -0.768178), glm::vec2(0.65625, 0.625) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, -0.653281), glm::vec2(0.625, 0.625) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, -0.162212), glm::vec2(0.65625, 0.0625) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, -0.31819), glm::vec2(0.65625, 0.125) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, -0.270598), glm::vec2(0.625, 0.125) });
	verts.push_back({ glm::vec3(0.544895, 0.19509, -0.815493), glm::vec2(0.65625, 0.5625) });
	verts.push_back({ glm::vec3(0.69352, 0.19509, -0.69352), glm::vec2(0.625, 0.5625) });
	verts.push_back({ glm::vec3(0.707106, 0, -0.707107), glm::vec2(0.625, 0.5) });
	verts.push_back({ glm::vec3(0.544895, -0.19509, -0.815493), glm::vec2(0.65625, 0.4375) });
	verts.push_back({ glm::vec3(0.55557, 0, -0.831469), glm::vec2(0.65625, 0.5) });
	verts.push_back({ glm::vec3(0.707106, 0, -0.707107), glm::vec2(0.625, 0.5) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, -0.162212), glm::vec2(0.65625, 0.9375) });
	verts.push_back({ glm::vec3(0.137949, 0.980785, -0.13795), glm::vec2(0.625, 0.9375) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, -0.270598), glm::vec2(0.625, 0.875) });
	verts.push_back({ glm::vec3(0.544895, -0.19509, -0.815493), glm::vec2(0.65625, 0.4375) });
	verts.push_back({ glm::vec3(0.69352, -0.19509, -0.69352), glm::vec2(0.625, 0.4375) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, -0.653281), glm::vec2(0.625, 0.375) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, -0.46194), glm::vec2(0.65625, 0.8125) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, -0.31819), glm::vec2(0.65625, 0.875) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, -0.270598), glm::vec2(0.625, 0.875) });
	verts.push_back({ glm::vec3(0.46194, -0.55557, -0.691342), glm::vec2(0.65625, 0.3125) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, -0.768178), glm::vec2(0.65625, 0.375) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, -0.653281), glm::vec2(0.625, 0.375) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, -0.587938), glm::vec2(0.65625, 0.75) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, -0.46194), glm::vec2(0.65625, 0.8125) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, -0.392847), glm::vec2(0.625, 0.8125) });
	verts.push_back({ glm::vec3(0.707106, 0, -0.707107), glm::vec2(0.625, 0.5) });
	verts.push_back({ glm::vec3(0.831469, 0, -0.55557), glm::vec2(0.59375, 0.5) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, -0.544895), glm::vec2(0.59375, 0.4375) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, -0.270598), glm::vec2(0.625, 0.875) });
	verts.push_back({ glm::vec3(0.137949, 0.980785, -0.13795), glm::vec2(0.625, 0.9375) });
	verts.push_back({ glm::vec3(0.162212, 0.980785, -0.108386), glm::vec2(0.59375, 0.9375) });
	verts.push_back({ glm::vec3(0.69352, -0.19509, -0.69352), glm::vec2(0.625, 0.4375) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, -0.544895), glm::vec2(0.59375, 0.4375) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, -0.51328), glm::vec2(0.59375, 0.375) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, -0.392847), glm::vec2(0.625, 0.8125) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, -0.270598), glm::vec2(0.625, 0.875) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, -0.212608), glm::vec2(0.59375, 0.875) });
	verts.push_back({ glm::vec3(0.587938, -0.55557, -0.587938), glm::vec2(0.625, 0.3125) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, -0.653281), glm::vec2(0.625, 0.375) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, -0.51328), glm::vec2(0.59375, 0.375) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, -0.392847), glm::vec2(0.625, 0.8125) });
	verts.push_back({ glm::vec3(0.46194, 0.83147, -0.308658), glm::vec2(0.59375, 0.8125) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, -0.392847), glm::vec2(0.59375, 0.75) });
	verts.push_back({ glm::vec3(0.587938, -0.55557, -0.587938), glm::vec2(0.625, 0.3125) });
	verts.push_back({ glm::vec3(0.691342, -0.55557, -0.46194), glm::vec2(0.59375, 0.3125) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, -0.392847), glm::vec2(0.59375, 0.25) });
	verts.push_back({ glm::vec3(0.587938, 0.55557, -0.587938), glm::vec2(0.625, 0.6875) });
	verts.push_back({ glm::vec3(0.5, 0.707107, -0.5), glm::vec2(0.625, 0.75) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, -0.392847), glm::vec2(0.59375, 0.75) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, -0.392847), glm::vec2(0.625, 0.1875) });
	verts.push_back({ glm::vec3(0.5, -0.707107, -0.5), glm::vec2(0.625, 0.25) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, -0.392847), glm::vec2(0.59375, 0.25) });
	verts.push_back({ glm::vec3(0.587938, 0.55557, -0.587938), glm::vec2(0.625, 0.6875) });
	verts.push_back({ glm::vec3(0.691342, 0.55557, -0.46194), glm::vec2(0.59375, 0.6875) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, -0.51328), glm::vec2(0.59375, 0.625) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, -0.392847), glm::vec2(0.625, 0.1875) });
	verts.push_back({ glm::vec3(0.46194, -0.83147, -0.308658), glm::vec2(0.59375, 0.1875) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, -0.212608), glm::vec2(0.59375, 0.125) });
	verts.push_back({ glm::vec3(0.69352, 0.19509, -0.69352), glm::vec2(0.625, 0.5625) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, -0.653281), glm::vec2(0.625, 0.625) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, -0.51328), glm::vec2(0.59375, 0.625) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, -0.270598), glm::vec2(0.625, 0.125) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, -0.212608), glm::vec2(0.59375, 0.125) });
	verts.push_back({ glm::vec3(0.162212, -0.980785, -0.108386), glm::vec2(0.59375, 0.0625) });
	verts.push_back({ glm::vec3(0.69352, 0.19509, -0.69352), glm::vec2(0.625, 0.5625) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, -0.544895), glm::vec2(0.59375, 0.5625) });
	verts.push_back({ glm::vec3(0.831469, 0, -0.55557), glm::vec2(0.59375, 0.5) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, -0.392847), glm::vec2(0.59375, 0.75) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, -0.270598), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, -0.31819), glm::vec2(0.5625, 0.6875) });
	verts.push_back({ glm::vec3(0.46194, -0.83147, -0.308658), glm::vec2(0.59375, 0.1875) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, -0.392847), glm::vec2(0.59375, 0.25) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, -0.270598), glm::vec2(0.5625, 0.25) });
	verts.push_back({ glm::vec3(0.691342, 0.55557, -0.46194), glm::vec2(0.59375, 0.6875) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, -0.31819), glm::vec2(0.5625, 0.6875) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, -0.353553), glm::vec2(0.5625, 0.625) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, -0.212608), glm::vec2(0.59375, 0.125) });
	verts.push_back({ glm::vec3(0.46194, -0.83147, -0.308658), glm::vec2(0.59375, 0.1875) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, -0.212607), glm::vec2(0.5625, 0.1875) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, -0.544895), glm::vec2(0.59375, 0.5625) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, -0.51328), glm::vec2(0.59375, 0.625) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, -0.353553), glm::vec2(0.5625, 0.625) });
	verts.push_back({ glm::vec3(0.162212, -0.980785, -0.108386), glm::vec2(0.59375, 0.0625) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, -0.212608), glm::vec2(0.59375, 0.125) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, -0.146447), glm::vec2(0.5625, 0.125) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, -0.544895), glm::vec2(0.59375, 0.5625) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, -0.37533), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.923879, 0, -0.382683), glm::vec2(0.5625, 0.5) });
	verts.push_back({ glm::vec3(0.831469, 0, -0.55557), glm::vec2(0.59375, 0.5) });
	verts.push_back({ glm::vec3(0.923879, 0, -0.382683), glm::vec2(0.5625, 0.5) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, -0.37533), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(0.162212, 0.980785, -0.108386), glm::vec2(0.59375, 0.9375) });
	verts.push_back({ glm::vec3(0.18024, 0.980785, -0.074658), glm::vec2(0.5625, 0.9375) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, -0.146447), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, -0.544895), glm::vec2(0.59375, 0.4375) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, -0.37533), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, -0.353553), glm::vec2(0.5625, 0.375) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, -0.212608), glm::vec2(0.59375, 0.875) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, -0.146447), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, -0.212607), glm::vec2(0.5625, 0.8125) });
	verts.push_back({ glm::vec3(0.691342, -0.55557, -0.46194), glm::vec2(0.59375, 0.3125) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, -0.51328), glm::vec2(0.59375, 0.375) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, -0.353553), glm::vec2(0.5625, 0.375) });
	verts.push_back({ glm::vec3(0.46194, 0.83147, -0.308658), glm::vec2(0.59375, 0.8125) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, -0.212607), glm::vec2(0.5625, 0.8125) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, -0.270598), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, -0.392847), glm::vec2(0.59375, 0.25) });
	verts.push_back({ glm::vec3(0.691342, -0.55557, -0.46194), glm::vec2(0.59375, 0.3125) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, -0.31819), glm::vec2(0.5625, 0.3125) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, -0.37533), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(0.923879, 0, -0.382683), glm::vec2(0.5625, 0.5) });
	verts.push_back({ glm::vec3(0.980785, 0, -0.19509), glm::vec2(0.53125, 0.5) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, -0.146447), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(0.18024, 0.980785, -0.074658), glm::vec2(0.5625, 0.9375) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, -0.03806), glm::vec2(0.53125, 0.9375) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, -0.37533), glm::vec2(0.5625, 0.4375) });
	verts.push_back({ glm::vec3(0.96194, -0.19509, -0.191342), glm::vec2(0.53125, 0.4375) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, -0.18024), glm::vec2(0.53125, 0.375) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, -0.212607), glm::vec2(0.5625, 0.8125) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, -0.146447), glm::vec2(0.5625, 0.875) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, -0.074658), glm::vec2(0.53125, 0.875) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, -0.31819), glm::vec2(0.5625, 0.3125) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, -0.353553), glm::vec2(0.5625, 0.375) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, -0.18024), glm::vec2(0.53125, 0.375) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, -0.270598), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, -0.212607), glm::vec2(0.5625, 0.8125) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, -0.108386), glm::vec2(0.53125, 0.8125) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, -0.31819), glm::vec2(0.5625, 0.3125) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, -0.162212), glm::vec2(0.53125, 0.3125) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, -0.13795), glm::vec2(0.53125, 0.25) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, -0.31819), glm::vec2(0.5625, 0.6875) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, -0.270598), glm::vec2(0.5625, 0.75) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, -0.13795), glm::vec2(0.53125, 0.75) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, -0.270598), glm::vec2(0.5625, 0.25) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, -0.13795), glm::vec2(0.53125, 0.25) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, -0.108386), glm::vec2(0.53125, 0.1875) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, -0.31819), glm::vec2(0.5625, 0.6875) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, -0.162212), glm::vec2(0.53125, 0.6875) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, -0.18024), glm::vec2(0.53125, 0.625) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, -0.212607), glm::vec2(0.5625, 0.1875) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, -0.108386), glm::vec2(0.53125, 0.1875) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, -0.074658), glm::vec2(0.53125, 0.125) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, -0.37533), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, -0.353553), glm::vec2(0.5625, 0.625) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, -0.18024), glm::vec2(0.53125, 0.625) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, -0.146447), glm::vec2(0.5625, 0.125) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, -0.074658), glm::vec2(0.53125, 0.125) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, -0.03806), glm::vec2(0.53125, 0.0625) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, -0.37533), glm::vec2(0.5625, 0.5625) });
	verts.push_back({ glm::vec3(0.96194, 0.19509, -0.191342), glm::vec2(0.53125, 0.5625) });
	verts.push_back({ glm::vec3(0.980785, 0, -0.19509), glm::vec2(0.53125, 0.5) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, -0.108386), glm::vec2(0.53125, 0.1875) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, -0.13795), glm::vec2(0.53125, 0.25) });
	verts.push_back({ glm::vec3(0.707106, -0.707107, 0), glm::vec2(0.5, 0.25) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, -0.162212), glm::vec2(0.53125, 0.6875) });
	verts.push_back({ glm::vec3(0.831469, 0.55557, 0), glm::vec2(0.5, 0.6875) });
	verts.push_back({ glm::vec3(0.923879, 0.382683, 0), glm::vec2(0.5, 0.625) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, -0.074658), glm::vec2(0.53125, 0.125) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, -0.108386), glm::vec2(0.53125, 0.1875) });
	verts.push_back({ glm::vec3(0.55557, -0.83147, 0), glm::vec2(0.5, 0.1875) });
	verts.push_back({ glm::vec3(0.96194, 0.19509, -0.191342), glm::vec2(0.53125, 0.5625) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, -0.18024), glm::vec2(0.53125, 0.625) });
	verts.push_back({ glm::vec3(0.923879, 0.382683, 0), glm::vec2(0.5, 0.625) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, -0.03806), glm::vec2(0.53125, 0.0625) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, -0.074658), glm::vec2(0.53125, 0.125) });
	verts.push_back({ glm::vec3(0.382683, -0.92388, 0), glm::vec2(0.5, 0.125) });
	verts.push_back({ glm::vec3(0.96194, 0.19509, -0.191342), glm::vec2(0.53125, 0.5625) });
	verts.push_back({ glm::vec3(0.980785, 0.19509, 0), glm::vec2(0.5, 0.5625) });
	verts.push_back({ glm::vec3(0.999999, 0, 0), glm::vec2(0.5, 0.5) });
	verts.push_back({ glm::vec3(0.980785, 0, -0.19509), glm::vec2(0.53125, 0.5) });
	verts.push_back({ glm::vec3(0.999999, 0, 0), glm::vec2(0.5, 0.5) });
	verts.push_back({ glm::vec3(0.980785, -0.19509, 0), glm::vec2(0.5, 0.4375) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, -0.03806), glm::vec2(0.53125, 0.9375) });
	verts.push_back({ glm::vec3(0.19509, 0.980785, 0), glm::vec2(0.5, 0.9375) });
	verts.push_back({ glm::vec3(0.382683, 0.92388, 0), glm::vec2(0.5, 0.875) });
	verts.push_back({ glm::vec3(0.96194, -0.19509, -0.191342), glm::vec2(0.53125, 0.4375) });
	verts.push_back({ glm::vec3(0.980785, -0.19509, 0), glm::vec2(0.5, 0.4375) });
	verts.push_back({ glm::vec3(0.923879, -0.382683, 0), glm::vec2(0.5, 0.375) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, -0.074658), glm::vec2(0.53125, 0.875) });
	verts.push_back({ glm::vec3(0.382683, 0.92388, 0), glm::vec2(0.5, 0.875) });
	verts.push_back({ glm::vec3(0.55557, 0.83147, 0), glm::vec2(0.5, 0.8125) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, -0.162212), glm::vec2(0.53125, 0.3125) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, -0.18024), glm::vec2(0.53125, 0.375) });
	verts.push_back({ glm::vec3(0.923879, -0.382683, 0), glm::vec2(0.5, 0.375) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, -0.108386), glm::vec2(0.53125, 0.8125) });
	verts.push_back({ glm::vec3(0.55557, 0.83147, 0), glm::vec2(0.5, 0.8125) });
	verts.push_back({ glm::vec3(0.707106, 0.707107, 0), glm::vec2(0.5, 0.75) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, -0.13795), glm::vec2(0.53125, 0.25) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, -0.162212), glm::vec2(0.53125, 0.3125) });
	verts.push_back({ glm::vec3(0.831469, -0.55557, 0), glm::vec2(0.5, 0.3125) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, -0.13795), glm::vec2(0.53125, 0.75) });
	verts.push_back({ glm::vec3(0.707106, 0.707107, 0), glm::vec2(0.5, 0.75) });
	verts.push_back({ glm::vec3(0.831469, 0.55557, 0), glm::vec2(0.5, 0.6875) });
	verts.push_back({ glm::vec3(0.382683, 0.92388, 0), glm::vec2(0.5, 0.875) });
	verts.push_back({ glm::vec3(0.19509, 0.980785, 0), glm::vec2(0.5, 0.9375) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, 0.03806), glm::vec2(0.46875, 0.9375) });
	verts.push_back({ glm::vec3(0.980785, -0.19509, 0), glm::vec2(0.5, 0.4375) });
	verts.push_back({ glm::vec3(0.961939, -0.19509, 0.191342), glm::vec2(0.46875, 0.4375) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, 0.18024), glm::vec2(0.46875, 0.375) });
	verts.push_back({ glm::vec3(0.55557, 0.83147, 0), glm::vec2(0.5, 0.8125) });
	verts.push_back({ glm::vec3(0.382683, 0.92388, 0), glm::vec2(0.5, 0.875) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, 0.074658), glm::vec2(0.46875, 0.875) });
	verts.push_back({ glm::vec3(0.831469, -0.55557, 0), glm::vec2(0.5, 0.3125) });
	verts.push_back({ glm::vec3(0.923879, -0.382683, 0), glm::vec2(0.5, 0.375) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, 0.18024), glm::vec2(0.46875, 0.375) });
	verts.push_back({ glm::vec3(0.707106, 0.707107, 0), glm::vec2(0.5, 0.75) });
	verts.push_back({ glm::vec3(0.55557, 0.83147, 0), glm::vec2(0.5, 0.8125) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, 0.108386), glm::vec2(0.46875, 0.8125) });
	verts.push_back({ glm::vec3(0.831469, -0.55557, 0), glm::vec2(0.5, 0.3125) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, 0.162212), glm::vec2(0.46875, 0.3125) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, 0.13795), glm::vec2(0.46875, 0.25) });
	verts.push_back({ glm::vec3(0.831469, 0.55557, 0), glm::vec2(0.5, 0.6875) });
	verts.push_back({ glm::vec3(0.707106, 0.707107, 0), glm::vec2(0.5, 0.75) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, 0.13795), glm::vec2(0.46875, 0.75) });
	verts.push_back({ glm::vec3(0.707106, -0.707107, 0), glm::vec2(0.5, 0.25) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, 0.13795), glm::vec2(0.46875, 0.25) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, 0.108386), glm::vec2(0.46875, 0.1875) });
	verts.push_back({ glm::vec3(0.831469, 0.55557, 0), glm::vec2(0.5, 0.6875) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, 0.162212), glm::vec2(0.46875, 0.6875) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, 0.18024), glm::vec2(0.46875, 0.625) });
	verts.push_back({ glm::vec3(0.55557, -0.83147, 0), glm::vec2(0.5, 0.1875) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, 0.108386), glm::vec2(0.46875, 0.1875) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, 0.074658), glm::vec2(0.46875, 0.125) });
	verts.push_back({ glm::vec3(0.980785, 0.19509, 0), glm::vec2(0.5, 0.5625) });
	verts.push_back({ glm::vec3(0.923879, 0.382683, 0), glm::vec2(0.5, 0.625) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, 0.18024), glm::vec2(0.46875, 0.625) });
	verts.push_back({ glm::vec3(0.382683, -0.92388, 0), glm::vec2(0.5, 0.125) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, 0.074658), glm::vec2(0.46875, 0.125) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, 0.03806), glm::vec2(0.46875, 0.0625) });
	verts.push_back({ glm::vec3(0.999999, 0, 0), glm::vec2(0.5, 0.5) });
	verts.push_back({ glm::vec3(0.980785, 0.19509, 0), glm::vec2(0.5, 0.5625) });
	verts.push_back({ glm::vec3(0.961939, 0.19509, 0.191342), glm::vec2(0.46875, 0.5625) });
	verts.push_back({ glm::vec3(0.999999, 0, 0), glm::vec2(0.5, 0.5) });
	verts.push_back({ glm::vec3(0.980785, 0, 0.19509), glm::vec2(0.46875, 0.5) });
	verts.push_back({ glm::vec3(0.961939, -0.19509, 0.191342), glm::vec2(0.46875, 0.4375) });
	verts.push_back({ glm::vec3(0.815493, 0.55557, 0.162212), glm::vec2(0.46875, 0.6875) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, 0.31819), glm::vec2(0.4375, 0.6875) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, 0.353553), glm::vec2(0.4375, 0.625) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, 0.074658), glm::vec2(0.46875, 0.125) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, 0.108386), glm::vec2(0.46875, 0.1875) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, 0.212608), glm::vec2(0.4375, 0.1875) });
	verts.push_back({ glm::vec3(0.961939, 0.19509, 0.191342), glm::vec2(0.46875, 0.5625) });
	verts.push_back({ glm::vec3(0.906127, 0.382683, 0.18024), glm::vec2(0.46875, 0.625) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, 0.353553), glm::vec2(0.4375, 0.625) });
	verts.push_back({ glm::vec3(0.191342, -0.980785, 0.03806), glm::vec2(0.46875, 0.0625) });
	verts.push_back({ glm::vec3(0.37533, -0.92388, 0.074658), glm::vec2(0.46875, 0.125) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, 0.146447), glm::vec2(0.4375, 0.125) });
	verts.push_back({ glm::vec3(0.980785, 0, 0.19509), glm::vec2(0.46875, 0.5) });
	verts.push_back({ glm::vec3(0.961939, 0.19509, 0.191342), glm::vec2(0.46875, 0.5625) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, 0.37533), glm::vec2(0.4375, 0.5625) });
	verts.push_back({ glm::vec3(0.980785, 0, 0.19509), glm::vec2(0.46875, 0.5) });
	verts.push_back({ glm::vec3(0.923879, 0, 0.382683), glm::vec2(0.4375, 0.5) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, 0.37533), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(0.191342, 0.980785, 0.03806), glm::vec2(0.46875, 0.9375) });
	verts.push_back({ glm::vec3(0.180239, 0.980785, 0.074658), glm::vec2(0.4375, 0.9375) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, 0.146447), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(0.961939, -0.19509, 0.191342), glm::vec2(0.46875, 0.4375) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, 0.37533), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, 0.353553), glm::vec2(0.4375, 0.375) });
	verts.push_back({ glm::vec3(0.37533, 0.92388, 0.074658), glm::vec2(0.46875, 0.875) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, 0.146447), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, 0.212608), glm::vec2(0.4375, 0.8125) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, 0.162212), glm::vec2(0.46875, 0.3125) });
	verts.push_back({ glm::vec3(0.906127, -0.382683, 0.18024), glm::vec2(0.46875, 0.375) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, 0.353553), glm::vec2(0.4375, 0.375) });
	verts.push_back({ glm::vec3(0.544895, 0.83147, 0.108386), glm::vec2(0.46875, 0.8125) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, 0.212608), glm::vec2(0.4375, 0.8125) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, 0.270598), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, 0.13795), glm::vec2(0.46875, 0.25) });
	verts.push_back({ glm::vec3(0.815493, -0.55557, 0.162212), glm::vec2(0.46875, 0.3125) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, 0.31819), glm::vec2(0.4375, 0.3125) });
	verts.push_back({ glm::vec3(0.69352, 0.707107, 0.13795), glm::vec2(0.46875, 0.75) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, 0.270598), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, 0.31819), glm::vec2(0.4375, 0.6875) });
	verts.push_back({ glm::vec3(0.544895, -0.83147, 0.108386), glm::vec2(0.46875, 0.1875) });
	verts.push_back({ glm::vec3(0.69352, -0.707107, 0.13795), glm::vec2(0.46875, 0.25) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, 0.270598), glm::vec2(0.4375, 0.25) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, 0.353553), glm::vec2(0.4375, 0.375) });
	verts.push_back({ glm::vec3(0.906127, -0.19509, 0.37533), glm::vec2(0.4375, 0.4375) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, 0.544895), glm::vec2(0.40625, 0.4375) });
	verts.push_back({ glm::vec3(0.353553, 0.92388, 0.146447), glm::vec2(0.4375, 0.875) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, 0.212608), glm::vec2(0.40625, 0.875) });
	verts.push_back({ glm::vec3(0.461939, 0.83147, 0.308658), glm::vec2(0.40625, 0.8125) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, 0.31819), glm::vec2(0.4375, 0.3125) });
	verts.push_back({ glm::vec3(0.853553, -0.382683, 0.353553), glm::vec2(0.4375, 0.375) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, 0.51328), glm::vec2(0.40625, 0.375) });
	verts.push_back({ glm::vec3(0.51328, 0.83147, 0.212608), glm::vec2(0.4375, 0.8125) });
	verts.push_back({ glm::vec3(0.461939, 0.83147, 0.308658), glm::vec2(0.40625, 0.8125) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, 0.392847), glm::vec2(0.40625, 0.75) });
	verts.push_back({ glm::vec3(0.768177, -0.55557, 0.31819), glm::vec2(0.4375, 0.3125) });
	verts.push_back({ glm::vec3(0.691341, -0.55557, 0.46194), glm::vec2(0.40625, 0.3125) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, 0.392847), glm::vec2(0.40625, 0.25) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, 0.31819), glm::vec2(0.4375, 0.6875) });
	verts.push_back({ glm::vec3(0.653281, 0.707107, 0.270598), glm::vec2(0.4375, 0.75) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, 0.392847), glm::vec2(0.40625, 0.75) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, 0.212608), glm::vec2(0.4375, 0.1875) });
	verts.push_back({ glm::vec3(0.653281, -0.707107, 0.270598), glm::vec2(0.4375, 0.25) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, 0.392847), glm::vec2(0.40625, 0.25) });
	verts.push_back({ glm::vec3(0.768177, 0.55557, 0.31819), glm::vec2(0.4375, 0.6875) });
	verts.push_back({ glm::vec3(0.691341, 0.55557, 0.46194), glm::vec2(0.40625, 0.6875) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, 0.51328), glm::vec2(0.40625, 0.625) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, 0.146447), glm::vec2(0.4375, 0.125) });
	verts.push_back({ glm::vec3(0.51328, -0.83147, 0.212608), glm::vec2(0.4375, 0.1875) });
	verts.push_back({ glm::vec3(0.461939, -0.83147, 0.308658), glm::vec2(0.40625, 0.1875) });
	verts.push_back({ glm::vec3(0.853553, 0.382683, 0.353553), glm::vec2(0.4375, 0.625) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, 0.51328), glm::vec2(0.40625, 0.625) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, 0.544895), glm::vec2(0.40625, 0.5625) });
	verts.push_back({ glm::vec3(0.180239, -0.980785, 0.074658), glm::vec2(0.4375, 0.0625) });
	verts.push_back({ glm::vec3(0.353553, -0.92388, 0.146447), glm::vec2(0.4375, 0.125) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, 0.212608), glm::vec2(0.40625, 0.125) });
	verts.push_back({ glm::vec3(0.923879, 0, 0.382683), glm::vec2(0.4375, 0.5) });
	verts.push_back({ glm::vec3(0.906127, 0.19509, 0.37533), glm::vec2(0.4375, 0.5625) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, 0.544895), glm::vec2(0.40625, 0.5625) });
	verts.push_back({ glm::vec3(0.923879, 0, 0.382683), glm::vec2(0.4375, 0.5) });
	verts.push_back({ glm::vec3(0.831469, 0, 0.55557), glm::vec2(0.40625, 0.5) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, 0.544895), glm::vec2(0.40625, 0.4375) });
	verts.push_back({ glm::vec3(0.180239, 0.980785, 0.074658), glm::vec2(0.4375, 0.9375) });
	verts.push_back({ glm::vec3(0.162211, 0.980785, 0.108386), glm::vec2(0.40625, 0.9375) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, 0.212608), glm::vec2(0.40625, 0.875) });
	verts.push_back({ glm::vec3(0.461939, -0.83147, 0.308658), glm::vec2(0.40625, 0.1875) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, 0.392847), glm::vec2(0.375, 0.1875) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, 0.270598), glm::vec2(0.375, 0.125) });
	verts.push_back({ glm::vec3(0.768177, 0.382683, 0.51328), glm::vec2(0.40625, 0.625) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, 0.653281), glm::vec2(0.375, 0.625) });
	verts.push_back({ glm::vec3(0.693519, 0.19509, 0.69352), glm::vec2(0.375, 0.5625) });
	verts.push_back({ glm::vec3(0.162211, -0.980785, 0.108386), glm::vec2(0.40625, 0.0625) });
	verts.push_back({ glm::vec3(0.318189, -0.92388, 0.212608), glm::vec2(0.40625, 0.125) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, 0.270598), glm::vec2(0.375, 0.125) });
	verts.push_back({ glm::vec3(0.831469, 0, 0.55557), glm::vec2(0.40625, 0.5) });
	verts.push_back({ glm::vec3(0.815493, 0.19509, 0.544895), glm::vec2(0.40625, 0.5625) });
	verts.push_back({ glm::vec3(0.693519, 0.19509, 0.69352), glm::vec2(0.375, 0.5625) });
	verts.push_back({ glm::vec3(0.831469, 0, 0.55557), glm::vec2(0.40625, 0.5) });
	verts.push_back({ glm::vec3(0.707106, 0, 0.707107), glm::vec2(0.375, 0.5) });
	verts.push_back({ glm::vec3(0.693519, -0.19509, 0.69352), glm::vec2(0.375, 0.4375) });
	verts.push_back({ glm::vec3(0.162211, 0.980785, 0.108386), glm::vec2(0.40625, 0.9375) });
	verts.push_back({ glm::vec3(0.137949, 0.980785, 0.13795), glm::vec2(0.375, 0.9375) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, 0.270598), glm::vec2(0.375, 0.875) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, 0.51328), glm::vec2(0.40625, 0.375) });
	verts.push_back({ glm::vec3(0.815493, -0.19509, 0.544895), glm::vec2(0.40625, 0.4375) });
	verts.push_back({ glm::vec3(0.693519, -0.19509, 0.69352), glm::vec2(0.375, 0.4375) });
	verts.push_back({ glm::vec3(0.461939, 0.83147, 0.308658), glm::vec2(0.40625, 0.8125) });
	verts.push_back({ glm::vec3(0.318189, 0.92388, 0.212608), glm::vec2(0.40625, 0.875) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, 0.270598), glm::vec2(0.375, 0.875) });
	verts.push_back({ glm::vec3(0.691341, -0.55557, 0.46194), glm::vec2(0.40625, 0.3125) });
	verts.push_back({ glm::vec3(0.768177, -0.382683, 0.51328), glm::vec2(0.40625, 0.375) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, 0.653281), glm::vec2(0.375, 0.375) });
	verts.push_back({ glm::vec3(0.461939, 0.83147, 0.308658), glm::vec2(0.40625, 0.8125) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, 0.392847), glm::vec2(0.375, 0.8125) });
	verts.push_back({ glm::vec3(0.5, 0.707107, 0.5), glm::vec2(0.375, 0.75) });
	verts.push_back({ glm::vec3(0.691341, -0.55557, 0.46194), glm::vec2(0.40625, 0.3125) });
	verts.push_back({ glm::vec3(0.587937, -0.55557, 0.587938), glm::vec2(0.375, 0.3125) });
	verts.push_back({ glm::vec3(0.5, -0.707107, 0.5), glm::vec2(0.375, 0.25) });
	verts.push_back({ glm::vec3(0.691341, 0.55557, 0.46194), glm::vec2(0.40625, 0.6875) });
	verts.push_back({ glm::vec3(0.587938, 0.707107, 0.392847), glm::vec2(0.40625, 0.75) });
	verts.push_back({ glm::vec3(0.5, 0.707107, 0.5), glm::vec2(0.375, 0.75) });
	verts.push_back({ glm::vec3(0.461939, -0.83147, 0.308658), glm::vec2(0.40625, 0.1875) });
	verts.push_back({ glm::vec3(0.587938, -0.707107, 0.392847), glm::vec2(0.40625, 0.25) });
	verts.push_back({ glm::vec3(0.5, -0.707107, 0.5), glm::vec2(0.375, 0.25) });
	verts.push_back({ glm::vec3(0.691341, 0.55557, 0.46194), glm::vec2(0.40625, 0.6875) });
	verts.push_back({ glm::vec3(0.587937, 0.55557, 0.587938), glm::vec2(0.375, 0.6875) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, 0.653281), glm::vec2(0.375, 0.625) });
	verts.push_back({ glm::vec3(0.587937, -0.55557, 0.587938), glm::vec2(0.375, 0.3125) });
	verts.push_back({ glm::vec3(0.653281, -0.382683, 0.653281), glm::vec2(0.375, 0.375) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, 0.768178), glm::vec2(0.34375, 0.375) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, 0.392847), glm::vec2(0.375, 0.8125) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, 0.46194), glm::vec2(0.34375, 0.8125) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, 0.587938), glm::vec2(0.34375, 0.75) });
	verts.push_back({ glm::vec3(0.587937, -0.55557, 0.587938), glm::vec2(0.375, 0.3125) });
	verts.push_back({ glm::vec3(0.461939, -0.55557, 0.691342), glm::vec2(0.34375, 0.3125) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, 0.587938), glm::vec2(0.34375, 0.25) });
	verts.push_back({ glm::vec3(0.587937, 0.55557, 0.587938), glm::vec2(0.375, 0.6875) });
	verts.push_back({ glm::vec3(0.5, 0.707107, 0.5), glm::vec2(0.375, 0.75) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, 0.587938), glm::vec2(0.34375, 0.75) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, 0.392847), glm::vec2(0.375, 0.1875) });
	verts.push_back({ glm::vec3(0.5, -0.707107, 0.5), glm::vec2(0.375, 0.25) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, 0.587938), glm::vec2(0.34375, 0.25) });
	verts.push_back({ glm::vec3(0.587937, 0.55557, 0.587938), glm::vec2(0.375, 0.6875) });
	verts.push_back({ glm::vec3(0.461939, 0.55557, 0.691342), glm::vec2(0.34375, 0.6875) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, 0.768178), glm::vec2(0.34375, 0.625) });
	verts.push_back({ glm::vec3(0.392847, -0.83147, 0.392847), glm::vec2(0.375, 0.1875) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, 0.46194), glm::vec2(0.34375, 0.1875) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, 0.31819), glm::vec2(0.34375, 0.125) });
	verts.push_back({ glm::vec3(0.693519, 0.19509, 0.69352), glm::vec2(0.375, 0.5625) });
	verts.push_back({ glm::vec3(0.653281, 0.382683, 0.653281), glm::vec2(0.375, 0.625) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, 0.768178), glm::vec2(0.34375, 0.625) });
	verts.push_back({ glm::vec3(0.270598, -0.92388, 0.270598), glm::vec2(0.375, 0.125) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, 0.31819), glm::vec2(0.34375, 0.125) });
	verts.push_back({ glm::vec3(0.108386, -0.980785, 0.162212), glm::vec2(0.34375, 0.0625) });
	verts.push_back({ glm::vec3(0.707106, 0, 0.707107), glm::vec2(0.375, 0.5) });
	verts.push_back({ glm::vec3(0.693519, 0.19509, 0.69352), glm::vec2(0.375, 0.5625) });
	verts.push_back({ glm::vec3(0.544894, 0.19509, 0.815493), glm::vec2(0.34375, 0.5625) });
	verts.push_back({ glm::vec3(0.707106, 0, 0.707107), glm::vec2(0.375, 0.5) });
	verts.push_back({ glm::vec3(0.555569, 0, 0.831469), glm::vec2(0.34375, 0.5) });
	verts.push_back({ glm::vec3(0.544894, -0.19509, 0.815493), glm::vec2(0.34375, 0.4375) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, 0.270598), glm::vec2(0.375, 0.875) });
	verts.push_back({ glm::vec3(0.137949, 0.980785, 0.13795), glm::vec2(0.375, 0.9375) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, 0.162212), glm::vec2(0.34375, 0.9375) });
	verts.push_back({ glm::vec3(0.693519, -0.19509, 0.69352), glm::vec2(0.375, 0.4375) });
	verts.push_back({ glm::vec3(0.544894, -0.19509, 0.815493), glm::vec2(0.34375, 0.4375) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, 0.768178), glm::vec2(0.34375, 0.375) });
	verts.push_back({ glm::vec3(0.392847, 0.83147, 0.392847), glm::vec2(0.375, 0.8125) });
	verts.push_back({ glm::vec3(0.270598, 0.92388, 0.270598), glm::vec2(0.375, 0.875) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, 0.31819), glm::vec2(0.34375, 0.875) });
	verts.push_back({ glm::vec3(0.212607, -0.92388, 0.31819), glm::vec2(0.34375, 0.125) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, 0.353553), glm::vec2(0.3125, 0.125) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, 0.18024), glm::vec2(0.3125, 0.0625) });
	verts.push_back({ glm::vec3(0.555569, 0, 0.831469), glm::vec2(0.34375, 0.5) });
	verts.push_back({ glm::vec3(0.544894, 0.19509, 0.815493), glm::vec2(0.34375, 0.5625) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, 0.906127), glm::vec2(0.3125, 0.5625) });
	verts.push_back({ glm::vec3(0.555569, 0, 0.831469), glm::vec2(0.34375, 0.5) });
	verts.push_back({ glm::vec3(0.382683, 0, 0.923879), glm::vec2(0.3125, 0.5) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, 0.906127), glm::vec2(0.3125, 0.4375) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, 0.31819), glm::vec2(0.34375, 0.875) });
	verts.push_back({ glm::vec3(0.108386, 0.980785, 0.162212), glm::vec2(0.34375, 0.9375) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, 0.18024), glm::vec2(0.3125, 0.9375) });
	verts.push_back({ glm::vec3(0.544894, -0.19509, 0.815493), glm::vec2(0.34375, 0.4375) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, 0.906127), glm::vec2(0.3125, 0.4375) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, 0.853553), glm::vec2(0.3125, 0.375) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, 0.46194), glm::vec2(0.34375, 0.8125) });
	verts.push_back({ glm::vec3(0.212607, 0.92388, 0.31819), glm::vec2(0.34375, 0.875) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, 0.353553), glm::vec2(0.3125, 0.875) });
	verts.push_back({ glm::vec3(0.461939, -0.55557, 0.691342), glm::vec2(0.34375, 0.3125) });
	verts.push_back({ glm::vec3(0.51328, -0.382683, 0.768178), glm::vec2(0.34375, 0.375) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, 0.853553), glm::vec2(0.3125, 0.375) });
	verts.push_back({ glm::vec3(0.308658, 0.83147, 0.46194), glm::vec2(0.34375, 0.8125) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, 0.51328), glm::vec2(0.3125, 0.8125) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, 0.653281), glm::vec2(0.3125, 0.75) });
	verts.push_back({ glm::vec3(0.461939, -0.55557, 0.691342), glm::vec2(0.34375, 0.3125) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, 0.768178), glm::vec2(0.3125, 0.3125) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, 0.653281), glm::vec2(0.3125, 0.25) });
	verts.push_back({ glm::vec3(0.461939, 0.55557, 0.691342), glm::vec2(0.34375, 0.6875) });
	verts.push_back({ glm::vec3(0.392847, 0.707107, 0.587938), glm::vec2(0.34375, 0.75) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, 0.653281), glm::vec2(0.3125, 0.75) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, 0.46194), glm::vec2(0.34375, 0.1875) });
	verts.push_back({ glm::vec3(0.392847, -0.707107, 0.587938), glm::vec2(0.34375, 0.25) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, 0.653281), glm::vec2(0.3125, 0.25) });
	verts.push_back({ glm::vec3(0.461939, 0.55557, 0.691342), glm::vec2(0.34375, 0.6875) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, 0.768178), glm::vec2(0.3125, 0.6875) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, 0.853553), glm::vec2(0.3125, 0.625) });
	verts.push_back({ glm::vec3(0.308658, -0.83147, 0.46194), glm::vec2(0.34375, 0.1875) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, 0.51328), glm::vec2(0.3125, 0.1875) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, 0.353553), glm::vec2(0.3125, 0.125) });
	verts.push_back({ glm::vec3(0.544894, 0.19509, 0.815493), glm::vec2(0.34375, 0.5625) });
	verts.push_back({ glm::vec3(0.51328, 0.382683, 0.768178), glm::vec2(0.34375, 0.625) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, 0.853553), glm::vec2(0.3125, 0.625) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, 0.51328), glm::vec2(0.3125, 0.8125) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, 0.544895), glm::vec2(0.28125, 0.8125) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, 0.69352), glm::vec2(0.28125, 0.75) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, 0.653281), glm::vec2(0.3125, 0.25) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, 0.768178), glm::vec2(0.3125, 0.3125) });
	verts.push_back({ glm::vec3(0.162211, -0.55557, 0.815493), glm::vec2(0.28125, 0.3125) });
	verts.push_back({ glm::vec3(0.270598, 0.707107, 0.653281), glm::vec2(0.3125, 0.75) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, 0.69352), glm::vec2(0.28125, 0.75) });
	verts.push_back({ glm::vec3(0.162211, 0.55557, 0.815493), glm::vec2(0.28125, 0.6875) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, 0.51328), glm::vec2(0.3125, 0.1875) });
	verts.push_back({ glm::vec3(0.270598, -0.707107, 0.653281), glm::vec2(0.3125, 0.25) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, 0.69352), glm::vec2(0.28125, 0.25) });
	verts.push_back({ glm::vec3(0.318189, 0.55557, 0.768178), glm::vec2(0.3125, 0.6875) });
	verts.push_back({ glm::vec3(0.162211, 0.55557, 0.815493), glm::vec2(0.28125, 0.6875) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, 0.906127), glm::vec2(0.28125, 0.625) });
	verts.push_back({ glm::vec3(0.212607, -0.83147, 0.51328), glm::vec2(0.3125, 0.1875) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, 0.544895), glm::vec2(0.28125, 0.1875) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, 0.37533), glm::vec2(0.28125, 0.125) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, 0.906127), glm::vec2(0.3125, 0.5625) });
	verts.push_back({ glm::vec3(0.353553, 0.382683, 0.853553), glm::vec2(0.3125, 0.625) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, 0.906127), glm::vec2(0.28125, 0.625) });
	verts.push_back({ glm::vec3(0.074658, -0.980785, 0.18024), glm::vec2(0.3125, 0.0625) });
	verts.push_back({ glm::vec3(0.146446, -0.92388, 0.353553), glm::vec2(0.3125, 0.125) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, 0.37533), glm::vec2(0.28125, 0.125) });
	verts.push_back({ glm::vec3(0.382683, 0, 0.923879), glm::vec2(0.3125, 0.5) });
	verts.push_back({ glm::vec3(0.37533, 0.19509, 0.906127), glm::vec2(0.3125, 0.5625) });
	verts.push_back({ glm::vec3(0.191341, 0.19509, 0.961939), glm::vec2(0.28125, 0.5625) });
	verts.push_back({ glm::vec3(0.382683, 0, 0.923879), glm::vec2(0.3125, 0.5) });
	verts.push_back({ glm::vec3(0.19509, 0, 0.980785), glm::vec2(0.28125, 0.5) });
	verts.push_back({ glm::vec3(0.191341, -0.19509, 0.961939), glm::vec2(0.28125, 0.4375) });
	verts.push_back({ glm::vec3(0.074658, 0.980785, 0.18024), glm::vec2(0.3125, 0.9375) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, 0.191342), glm::vec2(0.28125, 0.9375) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, 0.37533), glm::vec2(0.28125, 0.875) });
	verts.push_back({ glm::vec3(0.37533, -0.19509, 0.906127), glm::vec2(0.3125, 0.4375) });
	verts.push_back({ glm::vec3(0.191341, -0.19509, 0.961939), glm::vec2(0.28125, 0.4375) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, 0.906127), glm::vec2(0.28125, 0.375) });
	verts.push_back({ glm::vec3(0.212607, 0.83147, 0.51328), glm::vec2(0.3125, 0.8125) });
	verts.push_back({ glm::vec3(0.146446, 0.92388, 0.353553), glm::vec2(0.3125, 0.875) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, 0.37533), glm::vec2(0.28125, 0.875) });
	verts.push_back({ glm::vec3(0.318189, -0.55557, 0.768178), glm::vec2(0.3125, 0.3125) });
	verts.push_back({ glm::vec3(0.353553, -0.382683, 0.853553), glm::vec2(0.3125, 0.375) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, 0.906127), glm::vec2(0.28125, 0.375) });
	verts.push_back({ glm::vec3(0.19509, 0, 0.980785), glm::vec2(0.28125, 0.5) });
	verts.push_back({ glm::vec3(0.191341, 0.19509, 0.961939), glm::vec2(0.28125, 0.5625) });
	verts.push_back({ glm::vec3(0, 0.19509, 0.980785), glm::vec2(0.25, 0.5625) });
	verts.push_back({ glm::vec3(0.19509, 0, 0.980785), glm::vec2(0.28125, 0.5) });
	verts.push_back({ glm::vec3(-1E-06, 0, 0.999999), glm::vec2(0.25, 0.5) });
	verts.push_back({ glm::vec3(0, -0.19509, 0.980785), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(0.03806, 0.980785, 0.191342), glm::vec2(0.28125, 0.9375) });
	verts.push_back({ glm::vec3(0, 0.980785, 0.19509), glm::vec2(0.25, 0.9375) });
	verts.push_back({ glm::vec3(0, 0.92388, 0.382683), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(0.191341, -0.19509, 0.961939), glm::vec2(0.28125, 0.4375) });
	verts.push_back({ glm::vec3(0, -0.19509, 0.980785), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(0, -0.382683, 0.923879), glm::vec2(0.25, 0.375) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, 0.544895), glm::vec2(0.28125, 0.8125) });
	verts.push_back({ glm::vec3(0.074658, 0.92388, 0.37533), glm::vec2(0.28125, 0.875) });
	verts.push_back({ glm::vec3(0, 0.92388, 0.382683), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(0.162211, -0.55557, 0.815493), glm::vec2(0.28125, 0.3125) });
	verts.push_back({ glm::vec3(0.18024, -0.382683, 0.906127), glm::vec2(0.28125, 0.375) });
	verts.push_back({ glm::vec3(0, -0.382683, 0.923879), glm::vec2(0.25, 0.375) });
	verts.push_back({ glm::vec3(0.108386, 0.83147, 0.544895), glm::vec2(0.28125, 0.8125) });
	verts.push_back({ glm::vec3(0, 0.83147, 0.55557), glm::vec2(0.25, 0.8125) });
	verts.push_back({ glm::vec3(0, 0.707107, 0.707107), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, 0.69352), glm::vec2(0.28125, 0.25) });
	verts.push_back({ glm::vec3(0.162211, -0.55557, 0.815493), glm::vec2(0.28125, 0.3125) });
	verts.push_back({ glm::vec3(0, -0.55557, 0.831469), glm::vec2(0.25, 0.3125) });
	verts.push_back({ glm::vec3(0.137949, 0.707107, 0.69352), glm::vec2(0.28125, 0.75) });
	verts.push_back({ glm::vec3(0, 0.707107, 0.707107), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(0, 0.55557, 0.831469), glm::vec2(0.25, 0.6875) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, 0.544895), glm::vec2(0.28125, 0.1875) });
	verts.push_back({ glm::vec3(0.137949, -0.707107, 0.69352), glm::vec2(0.28125, 0.25) });
	verts.push_back({ glm::vec3(0, -0.707107, 0.707107), glm::vec2(0.25, 0.25) });
	verts.push_back({ glm::vec3(0.162211, 0.55557, 0.815493), glm::vec2(0.28125, 0.6875) });
	verts.push_back({ glm::vec3(0, 0.55557, 0.831469), glm::vec2(0.25, 0.6875) });
	verts.push_back({ glm::vec3(0, 0.382683, 0.923879), glm::vec2(0.25, 0.625) });
	verts.push_back({ glm::vec3(0.108386, -0.83147, 0.544895), glm::vec2(0.28125, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.83147, 0.55557), glm::vec2(0.25, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.92388, 0.382683), glm::vec2(0.25, 0.125) });
	verts.push_back({ glm::vec3(0.191341, 0.19509, 0.961939), glm::vec2(0.28125, 0.5625) });
	verts.push_back({ glm::vec3(0.18024, 0.382683, 0.906127), glm::vec2(0.28125, 0.625) });
	verts.push_back({ glm::vec3(0, 0.382683, 0.923879), glm::vec2(0.25, 0.625) });
	verts.push_back({ glm::vec3(0.074658, -0.92388, 0.37533), glm::vec2(0.28125, 0.125) });
	verts.push_back({ glm::vec3(0, -0.92388, 0.382683), glm::vec2(0.25, 0.125) });
	verts.push_back({ glm::vec3(0, -0.980785, 0.19509), glm::vec2(0.25, 0.0625) });
	verts.push_back({ glm::vec3(0, -0.55557, 0.831469), glm::vec2(0.25, 0.3125) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, 0.815493), glm::vec2(0.21875, 0.3125) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, 0.69352), glm::vec2(0.21875, 0.25) });
	verts.push_back({ glm::vec3(0, 0.55557, 0.831469), glm::vec2(0.25, 0.6875) });
	verts.push_back({ glm::vec3(0, 0.707107, 0.707107), glm::vec2(0.25, 0.75) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, 0.69352), glm::vec2(0.21875, 0.75) });
	verts.push_back({ glm::vec3(0, -0.83147, 0.55557), glm::vec2(0.25, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.707107, 0.707107), glm::vec2(0.25, 0.25) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, 0.69352), glm::vec2(0.21875, 0.25) });
	verts.push_back({ glm::vec3(0, 0.55557, 0.831469), glm::vec2(0.25, 0.6875) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, 0.815493), glm::vec2(0.21875, 0.6875) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, 0.906127), glm::vec2(0.21875, 0.625) });
	verts.push_back({ glm::vec3(0, -0.83147, 0.55557), glm::vec2(0.25, 0.1875) });
	verts.push_back({ glm::vec3(-0.108387, -0.83147, 0.544895), glm::vec2(0.21875, 0.1875) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, 0.37533), glm::vec2(0.21875, 0.125) });
	verts.push_back({ glm::vec3(0, 0.19509, 0.980785), glm::vec2(0.25, 0.5625) });
	verts.push_back({ glm::vec3(0, 0.382683, 0.923879), glm::vec2(0.25, 0.625) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, 0.906127), glm::vec2(0.21875, 0.625) });
	verts.push_back({ glm::vec3(0, -0.92388, 0.382683), glm::vec2(0.25, 0.125) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, 0.37533), glm::vec2(0.21875, 0.125) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, 0.191342), glm::vec2(0.21875, 0.0625) });
	verts.push_back({ glm::vec3(-1E-06, 0, 0.999999), glm::vec2(0.25, 0.5) });
	verts.push_back({ glm::vec3(0, 0.19509, 0.980785), glm::vec2(0.25, 0.5625) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, 0.961939), glm::vec2(0.21875, 0.5625) });
	verts.push_back({ glm::vec3(-1E-06, 0, 0.999999), glm::vec2(0.25, 0.5) });
	verts.push_back({ glm::vec3(-0.195091, 0, 0.980785), glm::vec2(0.21875, 0.5) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, 0.961939), glm::vec2(0.21875, 0.4375) });
	verts.push_back({ glm::vec3(0, 0.92388, 0.382683), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(0, 0.980785, 0.19509), glm::vec2(0.25, 0.9375) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, 0.191342), glm::vec2(0.21875, 0.9375) });
	verts.push_back({ glm::vec3(0, -0.19509, 0.980785), glm::vec2(0.25, 0.4375) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, 0.961939), glm::vec2(0.21875, 0.4375) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, 0.906127), glm::vec2(0.21875, 0.375) });
	verts.push_back({ glm::vec3(0, 0.92388, 0.382683), glm::vec2(0.25, 0.875) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, 0.37533), glm::vec2(0.21875, 0.875) });
	verts.push_back({ glm::vec3(-0.108387, 0.83147, 0.544895), glm::vec2(0.21875, 0.8125) });
	verts.push_back({ glm::vec3(0, -0.55557, 0.831469), glm::vec2(0.25, 0.3125) });
	verts.push_back({ glm::vec3(0, -0.382683, 0.923879), glm::vec2(0.25, 0.375) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, 0.906127), glm::vec2(0.21875, 0.375) });
	verts.push_back({ glm::vec3(0, 0.83147, 0.55557), glm::vec2(0.25, 0.8125) });
	verts.push_back({ glm::vec3(-0.108387, 0.83147, 0.544895), glm::vec2(0.21875, 0.8125) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, 0.69352), glm::vec2(0.21875, 0.75) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, 0.961939), glm::vec2(0.21875, 0.4375) });
	verts.push_back({ glm::vec3(-0.195091, 0, 0.980785), glm::vec2(0.21875, 0.5) });
	verts.push_back({ glm::vec3(-0.382684, 0, 0.923879), glm::vec2(0.1875, 0.5) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, 0.37533), glm::vec2(0.21875, 0.875) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, 0.191342), glm::vec2(0.21875, 0.9375) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, 0.18024), glm::vec2(0.1875, 0.9375) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, 0.961939), glm::vec2(0.21875, 0.4375) });
	verts.push_back({ glm::vec3(-0.375331, -0.19509, 0.906127), glm::vec2(0.1875, 0.4375) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, 0.853553), glm::vec2(0.1875, 0.375) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, 0.37533), glm::vec2(0.21875, 0.875) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, 0.353553), glm::vec2(0.1875, 0.875) });
	verts.push_back({ glm::vec3(-0.212608, 0.83147, 0.51328), glm::vec2(0.1875, 0.8125) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, 0.815493), glm::vec2(0.21875, 0.3125) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, 0.906127), glm::vec2(0.21875, 0.375) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, 0.853553), glm::vec2(0.1875, 0.375) });
	verts.push_back({ glm::vec3(-0.108387, 0.83147, 0.544895), glm::vec2(0.21875, 0.8125) });
	verts.push_back({ glm::vec3(-0.212608, 0.83147, 0.51328), glm::vec2(0.1875, 0.8125) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, 0.653281), glm::vec2(0.1875, 0.75) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, 0.815493), glm::vec2(0.21875, 0.3125) });
	verts.push_back({ glm::vec3(-0.31819, -0.55557, 0.768177), glm::vec2(0.1875, 0.3125) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, 0.653281), glm::vec2(0.1875, 0.25) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, 0.815493), glm::vec2(0.21875, 0.6875) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, 0.69352), glm::vec2(0.21875, 0.75) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, 0.653281), glm::vec2(0.1875, 0.75) });
	verts.push_back({ glm::vec3(-0.108387, -0.83147, 0.544895), glm::vec2(0.21875, 0.1875) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, 0.69352), glm::vec2(0.21875, 0.25) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, 0.653281), glm::vec2(0.1875, 0.25) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, 0.815493), glm::vec2(0.21875, 0.6875) });
	verts.push_back({ glm::vec3(-0.31819, 0.55557, 0.768177), glm::vec2(0.1875, 0.6875) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, 0.853553), glm::vec2(0.1875, 0.625) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, 0.37533), glm::vec2(0.21875, 0.125) });
	verts.push_back({ glm::vec3(-0.108387, -0.83147, 0.544895), glm::vec2(0.21875, 0.1875) });
	verts.push_back({ glm::vec3(-0.212608, -0.83147, 0.51328), glm::vec2(0.1875, 0.1875) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, 0.961939), glm::vec2(0.21875, 0.5625) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, 0.906127), glm::vec2(0.21875, 0.625) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, 0.853553), glm::vec2(0.1875, 0.625) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, 0.37533), glm::vec2(0.21875, 0.125) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, 0.353553), glm::vec2(0.1875, 0.125) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, 0.18024), glm::vec2(0.1875, 0.0625) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, 0.961939), glm::vec2(0.21875, 0.5625) });
	verts.push_back({ glm::vec3(-0.375331, 0.19509, 0.906127), glm::vec2(0.1875, 0.5625) });
	verts.push_back({ glm::vec3(-0.382684, 0, 0.923879), glm::vec2(0.1875, 0.5) });
	verts.push_back({ glm::vec3(-0.31819, 0.55557, 0.768177), glm::vec2(0.1875, 0.6875) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, 0.653281), glm::vec2(0.1875, 0.75) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, 0.587938), glm::vec2(0.15625, 0.75) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, 0.653281), glm::vec2(0.1875, 0.25) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, 0.587938), glm::vec2(0.15625, 0.25) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, 0.461939), glm::vec2(0.15625, 0.1875) });
	verts.push_back({ glm::vec3(-0.31819, 0.55557, 0.768177), glm::vec2(0.1875, 0.6875) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, 0.691341), glm::vec2(0.15625, 0.6875) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, 0.768178), glm::vec2(0.15625, 0.625) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, 0.353553), glm::vec2(0.1875, 0.125) });
	verts.push_back({ glm::vec3(-0.212608, -0.83147, 0.51328), glm::vec2(0.1875, 0.1875) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, 0.461939), glm::vec2(0.15625, 0.1875) });
	verts.push_back({ glm::vec3(-0.375331, 0.19509, 0.906127), glm::vec2(0.1875, 0.5625) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, 0.853553), glm::vec2(0.1875, 0.625) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, 0.768178), glm::vec2(0.15625, 0.625) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, 0.18024), glm::vec2(0.1875, 0.0625) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, 0.353553), glm::vec2(0.1875, 0.125) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, 0.31819), glm::vec2(0.15625, 0.125) });
	verts.push_back({ glm::vec3(-0.375331, 0.19509, 0.906127), glm::vec2(0.1875, 0.5625) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, 0.815493), glm::vec2(0.15625, 0.5625) });
	verts.push_back({ glm::vec3(-0.55557, 0, 0.831469), glm::vec2(0.15625, 0.5) });
	verts.push_back({ glm::vec3(-0.382684, 0, 0.923879), glm::vec2(0.1875, 0.5) });
	verts.push_back({ glm::vec3(-0.55557, 0, 0.831469), glm::vec2(0.15625, 0.5) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, 0.815493), glm::vec2(0.15625, 0.4375) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, 0.18024), glm::vec2(0.1875, 0.9375) });
	verts.push_back({ glm::vec3(-0.108387, 0.980785, 0.162212), glm::vec2(0.15625, 0.9375) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, 0.31819), glm::vec2(0.15625, 0.875) });
	verts.push_back({ glm::vec3(-0.375331, -0.19509, 0.906127), glm::vec2(0.1875, 0.4375) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, 0.815493), glm::vec2(0.15625, 0.4375) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, 0.768178), glm::vec2(0.15625, 0.375) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, 0.353553), glm::vec2(0.1875, 0.875) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, 0.31819), glm::vec2(0.15625, 0.875) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, 0.461939), glm::vec2(0.15625, 0.8125) });
	verts.push_back({ glm::vec3(-0.31819, -0.55557, 0.768177), glm::vec2(0.1875, 0.3125) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, 0.853553), glm::vec2(0.1875, 0.375) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, 0.768178), glm::vec2(0.15625, 0.375) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, 0.653281), glm::vec2(0.1875, 0.75) });
	verts.push_back({ glm::vec3(-0.212608, 0.83147, 0.51328), glm::vec2(0.1875, 0.8125) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, 0.461939), glm::vec2(0.15625, 0.8125) });
	verts.push_back({ glm::vec3(-0.31819, -0.55557, 0.768177), glm::vec2(0.1875, 0.3125) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, 0.691341), glm::vec2(0.15625, 0.3125) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, 0.587938), glm::vec2(0.15625, 0.25) });
	verts.push_back({ glm::vec3(-0.55557, 0, 0.831469), glm::vec2(0.15625, 0.5) });
	verts.push_back({ glm::vec3(-0.707107, 0, 0.707106), glm::vec2(0.125, 0.5) });
	verts.push_back({ glm::vec3(-0.69352, -0.19509, 0.69352), glm::vec2(0.125, 0.4375) });
	verts.push_back({ glm::vec3(-0.108387, 0.980785, 0.162212), glm::vec2(0.15625, 0.9375) });
	verts.push_back({ glm::vec3(-0.13795, 0.980785, 0.13795), glm::vec2(0.125, 0.9375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, 0.270598), glm::vec2(0.125, 0.875) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, 0.815493), glm::vec2(0.15625, 0.4375) });
	verts.push_back({ glm::vec3(-0.69352, -0.19509, 0.69352), glm::vec2(0.125, 0.4375) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, 0.653281), glm::vec2(0.125, 0.375) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, 0.461939), glm::vec2(0.15625, 0.8125) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, 0.31819), glm::vec2(0.15625, 0.875) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, 0.270598), glm::vec2(0.125, 0.875) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, 0.691341), glm::vec2(0.15625, 0.3125) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, 0.768178), glm::vec2(0.15625, 0.375) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, 0.653281), glm::vec2(0.125, 0.375) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, 0.461939), glm::vec2(0.15625, 0.8125) });
	verts.push_back({ glm::vec3(-0.392848, 0.83147, 0.392847), glm::vec2(0.125, 0.8125) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, 0.5), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, 0.691341), glm::vec2(0.15625, 0.3125) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, 0.587937), glm::vec2(0.125, 0.3125) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, 0.5), glm::vec2(0.125, 0.25) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, 0.691341), glm::vec2(0.15625, 0.6875) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, 0.587938), glm::vec2(0.15625, 0.75) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, 0.5), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, 0.461939), glm::vec2(0.15625, 0.1875) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, 0.587938), glm::vec2(0.15625, 0.25) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, 0.5), glm::vec2(0.125, 0.25) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, 0.691341), glm::vec2(0.15625, 0.6875) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, 0.587937), glm::vec2(0.125, 0.6875) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, 0.653281), glm::vec2(0.125, 0.625) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, 0.461939), glm::vec2(0.15625, 0.1875) });
	verts.push_back({ glm::vec3(-0.392848, -0.83147, 0.392847), glm::vec2(0.125, 0.1875) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, 0.270598), glm::vec2(0.125, 0.125) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, 0.815493), glm::vec2(0.15625, 0.5625) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, 0.768178), glm::vec2(0.15625, 0.625) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, 0.653281), glm::vec2(0.125, 0.625) });
	verts.push_back({ glm::vec3(-0.108387, -0.980785, 0.162212), glm::vec2(0.15625, 0.0625) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, 0.31819), glm::vec2(0.15625, 0.125) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, 0.270598), glm::vec2(0.125, 0.125) });
	verts.push_back({ glm::vec3(-0.55557, 0, 0.831469), glm::vec2(0.15625, 0.5) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, 0.815493), glm::vec2(0.15625, 0.5625) });
	verts.push_back({ glm::vec3(-0.69352, 0.19509, 0.69352), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(-0.392848, -0.83147, 0.392847), glm::vec2(0.125, 0.1875) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, 0.5), glm::vec2(0.125, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, 0.392847), glm::vec2(0.09375, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, 0.587937), glm::vec2(0.125, 0.6875) });
	verts.push_back({ glm::vec3(-0.691342, 0.55557, 0.461939), glm::vec2(0.09375, 0.6875) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, 0.51328), glm::vec2(0.09375, 0.625) });
	verts.push_back({ glm::vec3(-0.392848, -0.83147, 0.392847), glm::vec2(0.125, 0.1875) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, 0.308658), glm::vec2(0.09375, 0.1875) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, 0.212607), glm::vec2(0.09375, 0.125) });
	verts.push_back({ glm::vec3(-0.69352, 0.19509, 0.69352), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, 0.653281), glm::vec2(0.125, 0.625) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, 0.51328), glm::vec2(0.09375, 0.625) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, 0.270598), glm::vec2(0.125, 0.125) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, 0.212607), glm::vec2(0.09375, 0.125) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, 0.108386), glm::vec2(0.09375, 0.0625) });
	verts.push_back({ glm::vec3(-0.707107, 0, 0.707106), glm::vec2(0.125, 0.5) });
	verts.push_back({ glm::vec3(-0.69352, 0.19509, 0.69352), glm::vec2(0.125, 0.5625) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, 0.544895), glm::vec2(0.09375, 0.5625) });
	verts.push_back({ glm::vec3(-0.707107, 0, 0.707106), glm::vec2(0.125, 0.5) });
	verts.push_back({ glm::vec3(-0.831469, 0, 0.555569), glm::vec2(0.09375, 0.5) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, 0.544895), glm::vec2(0.09375, 0.4375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, 0.270598), glm::vec2(0.125, 0.875) });
	verts.push_back({ glm::vec3(-0.13795, 0.980785, 0.13795), glm::vec2(0.125, 0.9375) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, 0.108386), glm::vec2(0.09375, 0.9375) });
	verts.push_back({ glm::vec3(-0.69352, -0.19509, 0.69352), glm::vec2(0.125, 0.4375) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, 0.544895), glm::vec2(0.09375, 0.4375) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, 0.51328), glm::vec2(0.09375, 0.375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, 0.270598), glm::vec2(0.125, 0.875) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, 0.212607), glm::vec2(0.09375, 0.875) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, 0.308658), glm::vec2(0.09375, 0.8125) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, 0.587937), glm::vec2(0.125, 0.3125) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, 0.653281), glm::vec2(0.125, 0.375) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, 0.51328), glm::vec2(0.09375, 0.375) });
	verts.push_back({ glm::vec3(-0.392848, 0.83147, 0.392847), glm::vec2(0.125, 0.8125) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, 0.308658), glm::vec2(0.09375, 0.8125) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, 0.392847), glm::vec2(0.09375, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, 0.587937), glm::vec2(0.125, 0.3125) });
	verts.push_back({ glm::vec3(-0.691342, -0.55557, 0.461939), glm::vec2(0.09375, 0.3125) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, 0.392847), glm::vec2(0.09375, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, 0.587937), glm::vec2(0.125, 0.6875) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, 0.5), glm::vec2(0.125, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, 0.392847), glm::vec2(0.09375, 0.75) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, 0.544895), glm::vec2(0.09375, 0.4375) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, 0.37533), glm::vec2(0.0625, 0.4375) });
	verts.push_back({ glm::vec3(-0.853554, -0.382683, 0.353553), glm::vec2(0.0625, 0.375) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, 0.212607), glm::vec2(0.09375, 0.875) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, 0.146447), glm::vec2(0.0625, 0.875) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, 0.212607), glm::vec2(0.0625, 0.8125) });
	verts.push_back({ glm::vec3(-0.691342, -0.55557, 0.461939), glm::vec2(0.09375, 0.3125) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, 0.51328), glm::vec2(0.09375, 0.375) });
	verts.push_back({ glm::vec3(-0.853554, -0.382683, 0.353553), glm::vec2(0.0625, 0.375) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, 0.308658), glm::vec2(0.09375, 0.8125) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, 0.212607), glm::vec2(0.0625, 0.8125) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, 0.270598), glm::vec2(0.0625, 0.75) });
	verts.push_back({ glm::vec3(-0.691342, -0.55557, 0.461939), glm::vec2(0.09375, 0.3125) });
	verts.push_back({ glm::vec3(-0.768178, -0.55557, 0.318189), glm::vec2(0.0625, 0.3125) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, 0.270598), glm::vec2(0.0625, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, 0.392847), glm::vec2(0.09375, 0.75) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, 0.270598), glm::vec2(0.0625, 0.75) });
	verts.push_back({ glm::vec3(-0.768178, 0.55557, 0.318189), glm::vec2(0.0625, 0.6875) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, 0.308658), glm::vec2(0.09375, 0.1875) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, 0.392847), glm::vec2(0.09375, 0.25) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, 0.270598), glm::vec2(0.0625, 0.25) });
	verts.push_back({ glm::vec3(-0.691342, 0.55557, 0.461939), glm::vec2(0.09375, 0.6875) });
	verts.push_back({ glm::vec3(-0.768178, 0.55557, 0.318189), glm::vec2(0.0625, 0.6875) });
	verts.push_back({ glm::vec3(-0.853554, 0.382683, 0.353553), glm::vec2(0.0625, 0.625) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, 0.212607), glm::vec2(0.09375, 0.125) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, 0.308658), glm::vec2(0.09375, 0.1875) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, 0.212607), glm::vec2(0.0625, 0.1875) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, 0.544895), glm::vec2(0.09375, 0.5625) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, 0.51328), glm::vec2(0.09375, 0.625) });
	verts.push_back({ glm::vec3(-0.853554, 0.382683, 0.353553), glm::vec2(0.0625, 0.625) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, 0.212607), glm::vec2(0.09375, 0.125) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, 0.146447), glm::vec2(0.0625, 0.125) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, 0.074658), glm::vec2(0.0625, 0.0625) });
	verts.push_back({ glm::vec3(-0.831469, 0, 0.555569), glm::vec2(0.09375, 0.5) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, 0.544895), glm::vec2(0.09375, 0.5625) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, 0.37533), glm::vec2(0.0625, 0.5625) });
	verts.push_back({ glm::vec3(-0.831469, 0, 0.555569), glm::vec2(0.09375, 0.5) });
	verts.push_back({ glm::vec3(-0.923879, 0, 0.382683), glm::vec2(0.0625, 0.5) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, 0.37533), glm::vec2(0.0625, 0.4375) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, 0.108386), glm::vec2(0.09375, 0.9375) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, 0.074658), glm::vec2(0.0625, 0.9375) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, 0.146447), glm::vec2(0.0625, 0.875) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, 0.212607), glm::vec2(0.0625, 0.1875) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, 0.108386), glm::vec2(0.03125, 0.1875) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, 0.074658), glm::vec2(0.03125, 0.125) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, 0.37533), glm::vec2(0.0625, 0.5625) });
	verts.push_back({ glm::vec3(-0.853554, 0.382683, 0.353553), glm::vec2(0.0625, 0.625) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, 0.18024), glm::vec2(0.03125, 0.625) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, 0.074658), glm::vec2(0.0625, 0.0625) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, 0.146447), glm::vec2(0.0625, 0.125) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, 0.074658), glm::vec2(0.03125, 0.125) });
	verts.push_back({ glm::vec3(-0.923879, 0, 0.382683), glm::vec2(0.0625, 0.5) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, 0.37533), glm::vec2(0.0625, 0.5625) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, 0.191341), glm::vec2(0.03125, 0.5625) });
	verts.push_back({ glm::vec3(-0.923879, 0, 0.382683), glm::vec2(0.0625, 0.5) });
	verts.push_back({ glm::vec3(-0.980785, 0, 0.19509), glm::vec2(0.03125, 0.5) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, 0.191341), glm::vec2(0.03125, 0.4375) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, 0.074658), glm::vec2(0.0625, 0.9375) });
	verts.push_back({ glm::vec3(-0.191342, 0.980785, 0.03806), glm::vec2(0.03125, 0.9375) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, 0.074658), glm::vec2(0.03125, 0.875) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, 0.37533), glm::vec2(0.0625, 0.4375) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, 0.191341), glm::vec2(0.03125, 0.4375) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, 0.18024), glm::vec2(0.03125, 0.375) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, 0.146447), glm::vec2(0.0625, 0.875) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, 0.074658), glm::vec2(0.03125, 0.875) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, 0.108386), glm::vec2(0.03125, 0.8125) });
	verts.push_back({ glm::vec3(-0.768178, -0.55557, 0.318189), glm::vec2(0.0625, 0.3125) });
	verts.push_back({ glm::vec3(-0.853554, -0.382683, 0.353553), glm::vec2(0.0625, 0.375) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, 0.18024), glm::vec2(0.03125, 0.375) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, 0.212607), glm::vec2(0.0625, 0.8125) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, 0.108386), glm::vec2(0.03125, 0.8125) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, 0.13795), glm::vec2(0.03125, 0.75) });
	verts.push_back({ glm::vec3(-0.768178, -0.55557, 0.318189), glm::vec2(0.0625, 0.3125) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, 0.162211), glm::vec2(0.03125, 0.3125) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, 0.13795), glm::vec2(0.03125, 0.25) });
	verts.push_back({ glm::vec3(-0.768178, 0.55557, 0.318189), glm::vec2(0.0625, 0.6875) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, 0.270598), glm::vec2(0.0625, 0.75) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, 0.13795), glm::vec2(0.03125, 0.75) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, 0.270598), glm::vec2(0.0625, 0.25) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, 0.13795), glm::vec2(0.03125, 0.25) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, 0.108386), glm::vec2(0.03125, 0.1875) });
	verts.push_back({ glm::vec3(-0.768178, 0.55557, 0.318189), glm::vec2(0.0625, 0.6875) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, 0.162211), glm::vec2(0.03125, 0.6875) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, 0.18024), glm::vec2(0.03125, 0.625) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, 0.108386), glm::vec2(0.03125, 0.8125) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, 0.074658), glm::vec2(0.03125, 0.875) });
	verts.push_back({ glm::vec3(-0.382684, 0.92388, 0), glm::vec2(0, 0.875) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, 0.162211), glm::vec2(0.03125, 0.3125) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, 0.18024), glm::vec2(0.03125, 0.375) });
	verts.push_back({ glm::vec3(-0.923879, -0.382683, 0), glm::vec2(0, 0.375) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, 0.108386), glm::vec2(0.03125, 0.8125) });
	verts.push_back({ glm::vec3(-0.55557, 0.83147, 0), glm::vec2(0, 0.8125) });
	verts.push_back({ glm::vec3(-0.707107, 0.707107, 0), glm::vec2(0, 0.75) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, 0.162211), glm::vec2(0.03125, 0.3125) });
	verts.push_back({ glm::vec3(-0.831469, -0.55557, 0), glm::vec2(0, 0.3125) });
	verts.push_back({ glm::vec3(-0.707107, -0.707107, 0), glm::vec2(0, 0.25) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, 0.162211), glm::vec2(0.03125, 0.6875) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, 0.13795), glm::vec2(0.03125, 0.75) });
	verts.push_back({ glm::vec3(-0.707107, 0.707107, 0), glm::vec2(0, 0.75) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, 0.108386), glm::vec2(0.03125, 0.1875) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, 0.13795), glm::vec2(0.03125, 0.25) });
	verts.push_back({ glm::vec3(-0.707107, -0.707107, 0), glm::vec2(0, 0.25) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, 0.162211), glm::vec2(0.03125, 0.6875) });
	verts.push_back({ glm::vec3(-0.831469, 0.55557, 0), glm::vec2(0, 0.6875) });
	verts.push_back({ glm::vec3(-0.923879, 0.382683, 0), glm::vec2(0, 0.625) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, 0.108386), glm::vec2(0.03125, 0.1875) });
	verts.push_back({ glm::vec3(-0.55557, -0.83147, 0), glm::vec2(0, 0.1875) });
	verts.push_back({ glm::vec3(-0.382684, -0.92388, 0), glm::vec2(0, 0.125) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, 0.191341), glm::vec2(0.03125, 0.5625) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, 0.18024), glm::vec2(0.03125, 0.625) });
	verts.push_back({ glm::vec3(-0.923879, 0.382683, 0), glm::vec2(0, 0.625) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, 0.074658), glm::vec2(0.03125, 0.125) });
	verts.push_back({ glm::vec3(-0.382684, -0.92388, 0), glm::vec2(0, 0.125) });
	verts.push_back({ glm::vec3(-0.195091, -0.980785, 0), glm::vec2(0, 0.0625) });
	verts.push_back({ glm::vec3(-0.980785, 0, 0.19509), glm::vec2(0.03125, 0.5) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, 0.191341), glm::vec2(0.03125, 0.5625) });
	verts.push_back({ glm::vec3(-0.980785, 0.19509, 0), glm::vec2(0, 0.5625) });
	verts.push_back({ glm::vec3(-0.980785, 0, 0.19509), glm::vec2(0.03125, 0.5) });
	verts.push_back({ glm::vec3(-0.999999, 0, 0), glm::vec2(0, 0.5) });
	verts.push_back({ glm::vec3(-0.980785, -0.19509, 0), glm::vec2(0, 0.4375) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, 0.074658), glm::vec2(0.03125, 0.875) });
	verts.push_back({ glm::vec3(-0.191342, 0.980785, 0.03806), glm::vec2(0.03125, 0.9375) });
	verts.push_back({ glm::vec3(-0.195091, 0.980785, 0), glm::vec2(0, 0.9375) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, 0.191341), glm::vec2(0.03125, 0.4375) });
	verts.push_back({ glm::vec3(-0.980785, -0.19509, 0), glm::vec2(0, 0.4375) });
	verts.push_back({ glm::vec3(-0.923879, -0.382683, 0), glm::vec2(0, 0.375) });
	verts.push_back({ glm::vec3(-0.980785, 0.19509, 0), glm::vec2(1, 0.5625) });
	verts.push_back({ glm::vec3(-0.923879, 0.382683, 0), glm::vec2(1, 0.625) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, -0.18024), glm::vec2(0.96875, 0.625) });
	verts.push_back({ glm::vec3(-0.195091, -0.980785, 0), glm::vec2(1, 0.0625) });
	verts.push_back({ glm::vec3(-0.382684, -0.92388, 0), glm::vec2(1, 0.125) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, -0.074658), glm::vec2(0.96875, 0.125) });
	verts.push_back({ glm::vec3(-0.999999, 0, 0), glm::vec2(1, 0.5) });
	verts.push_back({ glm::vec3(-0.980785, 0.19509, 0), glm::vec2(1, 0.5625) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, -0.191342), glm::vec2(0.96875, 0.5625) });
	verts.push_back({ glm::vec3(-0.999999, 0, 0), glm::vec2(1, 0.5) });
	verts.push_back({ glm::vec3(-0.980785, 0, -0.195091), glm::vec2(0.96875, 0.5) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, -0.191342), glm::vec2(0.96875, 0.4375) });
	verts.push_back({ glm::vec3(-0.195091, 0.980785, 0), glm::vec2(1, 0.9375) });
	verts.push_back({ glm::vec3(-0.191342, 0.980785, -0.03806), glm::vec2(0.96875, 0.9375) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, -0.074658), glm::vec2(0.96875, 0.875) });
	verts.push_back({ glm::vec3(-0.980785, -0.19509, 0), glm::vec2(1, 0.4375) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, -0.191342), glm::vec2(0.96875, 0.4375) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, -0.18024), glm::vec2(0.96875, 0.375) });
	verts.push_back({ glm::vec3(-0.55557, 0.83147, 0), glm::vec2(1, 0.8125) });
	verts.push_back({ glm::vec3(-0.382684, 0.92388, 0), glm::vec2(1, 0.875) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, -0.074658), glm::vec2(0.96875, 0.875) });
	verts.push_back({ glm::vec3(-0.831469, -0.55557, 0), glm::vec2(1, 0.3125) });
	verts.push_back({ glm::vec3(-0.923879, -0.382683, 0), glm::vec2(1, 0.375) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, -0.18024), glm::vec2(0.96875, 0.375) });
	verts.push_back({ glm::vec3(-0.55557, 0.83147, 0), glm::vec2(1, 0.8125) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, -0.108386), glm::vec2(0.96875, 0.8125) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, -0.13795), glm::vec2(0.96875, 0.75) });
	verts.push_back({ glm::vec3(-0.831469, -0.55557, 0), glm::vec2(1, 0.3125) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, -0.162212), glm::vec2(0.96875, 0.3125) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, -0.13795), glm::vec2(0.96875, 0.25) });
	verts.push_back({ glm::vec3(-0.831469, 0.55557, 0), glm::vec2(1, 0.6875) });
	verts.push_back({ glm::vec3(-0.707107, 0.707107, 0), glm::vec2(1, 0.75) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, -0.13795), glm::vec2(0.96875, 0.75) });
	verts.push_back({ glm::vec3(-0.55557, -0.83147, 0), glm::vec2(1, 0.1875) });
	verts.push_back({ glm::vec3(-0.707107, -0.707107, 0), glm::vec2(1, 0.25) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, -0.13795), glm::vec2(0.96875, 0.25) });
	verts.push_back({ glm::vec3(-0.831469, 0.55557, 0), glm::vec2(1, 0.6875) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, -0.162212), glm::vec2(0.96875, 0.6875) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, -0.18024), glm::vec2(0.96875, 0.625) });
	verts.push_back({ glm::vec3(-0.55557, -0.83147, 0), glm::vec2(1, 0.1875) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, -0.108386), glm::vec2(0.96875, 0.1875) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, -0.074658), glm::vec2(0.96875, 0.125) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, -0.162212), glm::vec2(0.96875, 0.3125) });
	verts.push_back({ glm::vec3(-0.906127, -0.382683, -0.18024), glm::vec2(0.96875, 0.375) });
	verts.push_back({ glm::vec3(-0.853553, -0.382683, -0.353553), glm::vec2(0.9375, 0.375) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, -0.108386), glm::vec2(0.96875, 0.8125) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, -0.212607), glm::vec2(0.9375, 0.8125) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, -0.270598), glm::vec2(0.9375, 0.75) });
	verts.push_back({ glm::vec3(-0.815493, -0.55557, -0.162212), glm::vec2(0.96875, 0.3125) });
	verts.push_back({ glm::vec3(-0.768177, -0.55557, -0.31819), glm::vec2(0.9375, 0.3125) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, -0.270598), glm::vec2(0.9375, 0.25) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, -0.162212), glm::vec2(0.96875, 0.6875) });
	verts.push_back({ glm::vec3(-0.69352, 0.707107, -0.13795), glm::vec2(0.96875, 0.75) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, -0.270598), glm::vec2(0.9375, 0.75) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, -0.108386), glm::vec2(0.96875, 0.1875) });
	verts.push_back({ glm::vec3(-0.69352, -0.707107, -0.13795), glm::vec2(0.96875, 0.25) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, -0.270598), glm::vec2(0.9375, 0.25) });
	verts.push_back({ glm::vec3(-0.815493, 0.55557, -0.162212), glm::vec2(0.96875, 0.6875) });
	verts.push_back({ glm::vec3(-0.768177, 0.55557, -0.31819), glm::vec2(0.9375, 0.6875) });
	verts.push_back({ glm::vec3(-0.853553, 0.382683, -0.353553), glm::vec2(0.9375, 0.625) });
	verts.push_back({ glm::vec3(-0.544895, -0.83147, -0.108386), glm::vec2(0.96875, 0.1875) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, -0.212607), glm::vec2(0.9375, 0.1875) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, -0.146447), glm::vec2(0.9375, 0.125) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, -0.191342), glm::vec2(0.96875, 0.5625) });
	verts.push_back({ glm::vec3(-0.906127, 0.382683, -0.18024), glm::vec2(0.96875, 0.625) });
	verts.push_back({ glm::vec3(-0.853553, 0.382683, -0.353553), glm::vec2(0.9375, 0.625) });
	verts.push_back({ glm::vec3(-0.37533, -0.92388, -0.074658), glm::vec2(0.96875, 0.125) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, -0.146447), glm::vec2(0.9375, 0.125) });
	verts.push_back({ glm::vec3(-0.18024, -0.980785, -0.074658), glm::vec2(0.9375, 0.0625) });
	verts.push_back({ glm::vec3(-0.980785, 0, -0.195091), glm::vec2(0.96875, 0.5) });
	verts.push_back({ glm::vec3(-0.96194, 0.19509, -0.191342), glm::vec2(0.96875, 0.5625) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, -0.37533), glm::vec2(0.9375, 0.5625) });
	verts.push_back({ glm::vec3(-0.980785, 0, -0.195091), glm::vec2(0.96875, 0.5) });
	verts.push_back({ glm::vec3(-0.923878, 0, -0.382683), glm::vec2(0.9375, 0.5) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, -0.37533), glm::vec2(0.9375, 0.4375) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, -0.074658), glm::vec2(0.96875, 0.875) });
	verts.push_back({ glm::vec3(-0.191342, 0.980785, -0.03806), glm::vec2(0.96875, 0.9375) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, -0.074658), glm::vec2(0.9375, 0.9375) });
	verts.push_back({ glm::vec3(-0.96194, -0.19509, -0.191342), glm::vec2(0.96875, 0.4375) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, -0.37533), glm::vec2(0.9375, 0.4375) });
	verts.push_back({ glm::vec3(-0.853553, -0.382683, -0.353553), glm::vec2(0.9375, 0.375) });
	verts.push_back({ glm::vec3(-0.544895, 0.83147, -0.108386), glm::vec2(0.96875, 0.8125) });
	verts.push_back({ glm::vec3(-0.37533, 0.92388, -0.074658), glm::vec2(0.96875, 0.875) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, -0.146447), glm::vec2(0.9375, 0.875) });
	verts.push_back({ glm::vec3(-0.353554, -0.92388, -0.146447), glm::vec2(0.9375, 0.125) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, -0.212607), glm::vec2(0.90625, 0.125) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, -0.108386), glm::vec2(0.90625, 0.0625) });
	verts.push_back({ glm::vec3(-0.923878, 0, -0.382683), glm::vec2(0.9375, 0.5) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, -0.37533), glm::vec2(0.9375, 0.5625) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, -0.544895), glm::vec2(0.90625, 0.5625) });
	verts.push_back({ glm::vec3(-0.923878, 0, -0.382683), glm::vec2(0.9375, 0.5) });
	verts.push_back({ glm::vec3(-0.831469, 0, -0.55557), glm::vec2(0.90625, 0.5) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, -0.544895), glm::vec2(0.90625, 0.4375) });
	verts.push_back({ glm::vec3(-0.18024, 0.980785, -0.074658), glm::vec2(0.9375, 0.9375) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, -0.108386), glm::vec2(0.90625, 0.9375) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, -0.212607), glm::vec2(0.90625, 0.875) });
	verts.push_back({ glm::vec3(-0.906127, -0.19509, -0.37533), glm::vec2(0.9375, 0.4375) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, -0.544895), glm::vec2(0.90625, 0.4375) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, -0.51328), glm::vec2(0.90625, 0.375) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, -0.212607), glm::vec2(0.9375, 0.8125) });
	verts.push_back({ glm::vec3(-0.353554, 0.92388, -0.146447), glm::vec2(0.9375, 0.875) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, -0.212607), glm::vec2(0.90625, 0.875) });
	verts.push_back({ glm::vec3(-0.768177, -0.55557, -0.31819), glm::vec2(0.9375, 0.3125) });
	verts.push_back({ glm::vec3(-0.853553, -0.382683, -0.353553), glm::vec2(0.9375, 0.375) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, -0.51328), glm::vec2(0.90625, 0.375) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, -0.270598), glm::vec2(0.9375, 0.75) });
	verts.push_back({ glm::vec3(-0.51328, 0.83147, -0.212607), glm::vec2(0.9375, 0.8125) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, -0.308658), glm::vec2(0.90625, 0.8125) });
	verts.push_back({ glm::vec3(-0.768177, -0.55557, -0.31819), glm::vec2(0.9375, 0.3125) });
	verts.push_back({ glm::vec3(-0.691341, -0.55557, -0.46194), glm::vec2(0.90625, 0.3125) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, -0.392847), glm::vec2(0.90625, 0.25) });
	verts.push_back({ glm::vec3(-0.768177, 0.55557, -0.31819), glm::vec2(0.9375, 0.6875) });
	verts.push_back({ glm::vec3(-0.653281, 0.707107, -0.270598), glm::vec2(0.9375, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, -0.392847), glm::vec2(0.90625, 0.75) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, -0.212607), glm::vec2(0.9375, 0.1875) });
	verts.push_back({ glm::vec3(-0.653281, -0.707107, -0.270598), glm::vec2(0.9375, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, -0.392847), glm::vec2(0.90625, 0.25) });
	verts.push_back({ glm::vec3(-0.768177, 0.55557, -0.31819), glm::vec2(0.9375, 0.6875) });
	verts.push_back({ glm::vec3(-0.691341, 0.55557, -0.46194), glm::vec2(0.90625, 0.6875) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, -0.51328), glm::vec2(0.90625, 0.625) });
	verts.push_back({ glm::vec3(-0.51328, -0.83147, -0.212607), glm::vec2(0.9375, 0.1875) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, -0.308658), glm::vec2(0.90625, 0.1875) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, -0.212607), glm::vec2(0.90625, 0.125) });
	verts.push_back({ glm::vec3(-0.906127, 0.19509, -0.37533), glm::vec2(0.9375, 0.5625) });
	verts.push_back({ glm::vec3(-0.853553, 0.382683, -0.353553), glm::vec2(0.9375, 0.625) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, -0.51328), glm::vec2(0.90625, 0.625) });
	verts.push_back({ glm::vec3(-0.46194, 0.83147, -0.308658), glm::vec2(0.90625, 0.8125) });
	verts.push_back({ glm::vec3(-0.392847, 0.83147, -0.392847), glm::vec2(0.875, 0.8125) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, -0.5), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(-0.691341, -0.55557, -0.46194), glm::vec2(0.90625, 0.3125) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, -0.587938), glm::vec2(0.875, 0.3125) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, -0.5), glm::vec2(0.875, 0.25) });
	verts.push_back({ glm::vec3(-0.691341, 0.55557, -0.46194), glm::vec2(0.90625, 0.6875) });
	verts.push_back({ glm::vec3(-0.587938, 0.707107, -0.392847), glm::vec2(0.90625, 0.75) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, -0.5), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, -0.308658), glm::vec2(0.90625, 0.1875) });
	verts.push_back({ glm::vec3(-0.587938, -0.707107, -0.392847), glm::vec2(0.90625, 0.25) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, -0.5), glm::vec2(0.875, 0.25) });
	verts.push_back({ glm::vec3(-0.691341, 0.55557, -0.46194), glm::vec2(0.90625, 0.6875) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, -0.587938), glm::vec2(0.875, 0.6875) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, -0.653281), glm::vec2(0.875, 0.625) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, -0.212607), glm::vec2(0.90625, 0.125) });
	verts.push_back({ glm::vec3(-0.46194, -0.83147, -0.308658), glm::vec2(0.90625, 0.1875) });
	verts.push_back({ glm::vec3(-0.392847, -0.83147, -0.392847), glm::vec2(0.875, 0.1875) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, -0.544895), glm::vec2(0.90625, 0.5625) });
	verts.push_back({ glm::vec3(-0.768178, 0.382683, -0.51328), glm::vec2(0.90625, 0.625) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, -0.653281), glm::vec2(0.875, 0.625) });
	verts.push_back({ glm::vec3(-0.162212, -0.980785, -0.108386), glm::vec2(0.90625, 0.0625) });
	verts.push_back({ glm::vec3(-0.31819, -0.92388, -0.212607), glm::vec2(0.90625, 0.125) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, -0.270598), glm::vec2(0.875, 0.125) });
	verts.push_back({ glm::vec3(-0.831469, 0, -0.55557), glm::vec2(0.90625, 0.5) });
	verts.push_back({ glm::vec3(-0.815493, 0.19509, -0.544895), glm::vec2(0.90625, 0.5625) });
	verts.push_back({ glm::vec3(-0.693519, 0.19509, -0.69352), glm::vec2(0.875, 0.5625) });
	verts.push_back({ glm::vec3(-0.831469, 0, -0.55557), glm::vec2(0.90625, 0.5) });
	verts.push_back({ glm::vec3(-0.707106, 0, -0.707106), glm::vec2(0.875, 0.5) });
	verts.push_back({ glm::vec3(-0.693519, -0.19509, -0.69352), glm::vec2(0.875, 0.4375) });
	verts.push_back({ glm::vec3(-0.162212, 0.980785, -0.108386), glm::vec2(0.90625, 0.9375) });
	verts.push_back({ glm::vec3(-0.13795, 0.980785, -0.13795), glm::vec2(0.875, 0.9375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, -0.270598), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-0.815493, -0.19509, -0.544895), glm::vec2(0.90625, 0.4375) });
	verts.push_back({ glm::vec3(-0.693519, -0.19509, -0.69352), glm::vec2(0.875, 0.4375) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, -0.653281), glm::vec2(0.875, 0.375) });
	verts.push_back({ glm::vec3(-0.31819, 0.92388, -0.212607), glm::vec2(0.90625, 0.875) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, -0.270598), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-0.392847, 0.83147, -0.392847), glm::vec2(0.875, 0.8125) });
	verts.push_back({ glm::vec3(-0.691341, -0.55557, -0.46194), glm::vec2(0.90625, 0.3125) });
	verts.push_back({ glm::vec3(-0.768178, -0.382683, -0.51328), glm::vec2(0.90625, 0.375) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, -0.653281), glm::vec2(0.875, 0.375) });
	verts.push_back({ glm::vec3(-0.707106, 0, -0.707106), glm::vec2(0.875, 0.5) });
	verts.push_back({ glm::vec3(-0.693519, 0.19509, -0.69352), glm::vec2(0.875, 0.5625) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, -0.815493), glm::vec2(0.84375, 0.5625) });
	verts.push_back({ glm::vec3(-0.707106, 0, -0.707106), glm::vec2(0.875, 0.5) });
	verts.push_back({ glm::vec3(-0.555569, 0, -0.831469), glm::vec2(0.84375, 0.5) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, -0.815493), glm::vec2(0.84375, 0.4375) });
	verts.push_back({ glm::vec3(-0.13795, 0.980785, -0.13795), glm::vec2(0.875, 0.9375) });
	verts.push_back({ glm::vec3(-0.108386, 0.980785, -0.162212), glm::vec2(0.84375, 0.9375) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, -0.31819), glm::vec2(0.84375, 0.875) });
	verts.push_back({ glm::vec3(-0.693519, -0.19509, -0.69352), glm::vec2(0.875, 0.4375) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, -0.815493), glm::vec2(0.84375, 0.4375) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, -0.768177), glm::vec2(0.84375, 0.375) });
	verts.push_back({ glm::vec3(-0.270598, 0.92388, -0.270598), glm::vec2(0.875, 0.875) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, -0.31819), glm::vec2(0.84375, 0.875) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, -0.461939), glm::vec2(0.84375, 0.8125) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, -0.587938), glm::vec2(0.875, 0.3125) });
	verts.push_back({ glm::vec3(-0.653281, -0.382683, -0.653281), glm::vec2(0.875, 0.375) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, -0.768177), glm::vec2(0.84375, 0.375) });
	verts.push_back({ glm::vec3(-0.392847, 0.83147, -0.392847), glm::vec2(0.875, 0.8125) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, -0.461939), glm::vec2(0.84375, 0.8125) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, -0.587938), glm::vec2(0.84375, 0.75) });
	verts.push_back({ glm::vec3(-0.587938, -0.55557, -0.587938), glm::vec2(0.875, 0.3125) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, -0.691341), glm::vec2(0.84375, 0.3125) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, -0.587938), glm::vec2(0.84375, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, -0.587938), glm::vec2(0.875, 0.6875) });
	verts.push_back({ glm::vec3(-0.5, 0.707107, -0.5), glm::vec2(0.875, 0.75) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, -0.587938), glm::vec2(0.84375, 0.75) });
	verts.push_back({ glm::vec3(-0.392847, -0.83147, -0.392847), glm::vec2(0.875, 0.1875) });
	verts.push_back({ glm::vec3(-0.5, -0.707107, -0.5), glm::vec2(0.875, 0.25) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, -0.587938), glm::vec2(0.84375, 0.25) });
	verts.push_back({ glm::vec3(-0.587938, 0.55557, -0.587938), glm::vec2(0.875, 0.6875) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, -0.691341), glm::vec2(0.84375, 0.6875) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, -0.768177), glm::vec2(0.84375, 0.625) });
	verts.push_back({ glm::vec3(-0.392847, -0.83147, -0.392847), glm::vec2(0.875, 0.1875) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, -0.461939), glm::vec2(0.84375, 0.1875) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, -0.31819), glm::vec2(0.84375, 0.125) });
	verts.push_back({ glm::vec3(-0.693519, 0.19509, -0.69352), glm::vec2(0.875, 0.5625) });
	verts.push_back({ glm::vec3(-0.653281, 0.382683, -0.653281), glm::vec2(0.875, 0.625) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, -0.768177), glm::vec2(0.84375, 0.625) });
	verts.push_back({ glm::vec3(-0.13795, -0.980785, -0.13795), glm::vec2(0.875, 0.0625) });
	verts.push_back({ glm::vec3(-0.270598, -0.92388, -0.270598), glm::vec2(0.875, 0.125) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, -0.31819), glm::vec2(0.84375, 0.125) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, -0.691341), glm::vec2(0.84375, 0.3125) });
	verts.push_back({ glm::vec3(-0.318189, -0.55557, -0.768177), glm::vec2(0.8125, 0.3125) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, -0.653281), glm::vec2(0.8125, 0.25) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, -0.691341), glm::vec2(0.84375, 0.6875) });
	verts.push_back({ glm::vec3(-0.392848, 0.707107, -0.587938), glm::vec2(0.84375, 0.75) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, -0.653281), glm::vec2(0.8125, 0.75) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, -0.461939), glm::vec2(0.84375, 0.1875) });
	verts.push_back({ glm::vec3(-0.392848, -0.707107, -0.587938), glm::vec2(0.84375, 0.25) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, -0.653281), glm::vec2(0.8125, 0.25) });
	verts.push_back({ glm::vec3(-0.46194, 0.55557, -0.691341), glm::vec2(0.84375, 0.6875) });
	verts.push_back({ glm::vec3(-0.318189, 0.55557, -0.768177), glm::vec2(0.8125, 0.6875) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, -0.853553), glm::vec2(0.8125, 0.625) });
	verts.push_back({ glm::vec3(-0.308658, -0.83147, -0.461939), glm::vec2(0.84375, 0.1875) });
	verts.push_back({ glm::vec3(-0.212607, -0.83147, -0.513279), glm::vec2(0.8125, 0.1875) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, -0.353553), glm::vec2(0.8125, 0.125) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, -0.815493), glm::vec2(0.84375, 0.5625) });
	verts.push_back({ glm::vec3(-0.51328, 0.382683, -0.768177), glm::vec2(0.84375, 0.625) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, -0.853553), glm::vec2(0.8125, 0.625) });
	verts.push_back({ glm::vec3(-0.212608, -0.92388, -0.31819), glm::vec2(0.84375, 0.125) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, -0.353553), glm::vec2(0.8125, 0.125) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, -0.18024), glm::vec2(0.8125, 0.0625) });
	verts.push_back({ glm::vec3(-0.555569, 0, -0.831469), glm::vec2(0.84375, 0.5) });
	verts.push_back({ glm::vec3(-0.544895, 0.19509, -0.815493), glm::vec2(0.84375, 0.5625) });
	verts.push_back({ glm::vec3(-0.37533, 0.19509, -0.906127), glm::vec2(0.8125, 0.5625) });
	verts.push_back({ glm::vec3(-0.555569, 0, -0.831469), glm::vec2(0.84375, 0.5) });
	verts.push_back({ glm::vec3(-0.382683, 0, -0.923879), glm::vec2(0.8125, 0.5) });
	verts.push_back({ glm::vec3(-0.37533, -0.19509, -0.906127), glm::vec2(0.8125, 0.4375) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, -0.31819), glm::vec2(0.84375, 0.875) });
	verts.push_back({ glm::vec3(-0.108386, 0.980785, -0.162212), glm::vec2(0.84375, 0.9375) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, -0.18024), glm::vec2(0.8125, 0.9375) });
	verts.push_back({ glm::vec3(-0.544895, -0.19509, -0.815493), glm::vec2(0.84375, 0.4375) });
	verts.push_back({ glm::vec3(-0.37533, -0.19509, -0.906127), glm::vec2(0.8125, 0.4375) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, -0.853553), glm::vec2(0.8125, 0.375) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, -0.461939), glm::vec2(0.84375, 0.8125) });
	verts.push_back({ glm::vec3(-0.212608, 0.92388, -0.31819), glm::vec2(0.84375, 0.875) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, -0.353553), glm::vec2(0.8125, 0.875) });
	verts.push_back({ glm::vec3(-0.46194, -0.55557, -0.691341), glm::vec2(0.84375, 0.3125) });
	verts.push_back({ glm::vec3(-0.51328, -0.382683, -0.768177), glm::vec2(0.84375, 0.375) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, -0.853553), glm::vec2(0.8125, 0.375) });
	verts.push_back({ glm::vec3(-0.308658, 0.83147, -0.461939), glm::vec2(0.84375, 0.8125) });
	verts.push_back({ glm::vec3(-0.212607, 0.83147, -0.513279), glm::vec2(0.8125, 0.8125) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, -0.653281), glm::vec2(0.8125, 0.75) });
	verts.push_back({ glm::vec3(-0.382683, 0, -0.923879), glm::vec2(0.8125, 0.5) });
	verts.push_back({ glm::vec3(-0.19509, 0, -0.980784), glm::vec2(0.78125, 0.5) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, -0.961939), glm::vec2(0.78125, 0.4375) });
	verts.push_back({ glm::vec3(-0.074658, 0.980785, -0.18024), glm::vec2(0.8125, 0.9375) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, -0.191342), glm::vec2(0.78125, 0.9375) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, -0.37533), glm::vec2(0.78125, 0.875) });
	verts.push_back({ glm::vec3(-0.37533, -0.19509, -0.906127), glm::vec2(0.8125, 0.4375) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, -0.961939), glm::vec2(0.78125, 0.4375) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, -0.906127), glm::vec2(0.78125, 0.375) });
	verts.push_back({ glm::vec3(-0.212607, 0.83147, -0.513279), glm::vec2(0.8125, 0.8125) });
	verts.push_back({ glm::vec3(-0.146447, 0.92388, -0.353553), glm::vec2(0.8125, 0.875) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, -0.37533), glm::vec2(0.78125, 0.875) });
	verts.push_back({ glm::vec3(-0.318189, -0.55557, -0.768177), glm::vec2(0.8125, 0.3125) });
	verts.push_back({ glm::vec3(-0.353554, -0.382683, -0.853553), glm::vec2(0.8125, 0.375) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, -0.906127), glm::vec2(0.78125, 0.375) });
	verts.push_back({ glm::vec3(-0.212607, 0.83147, -0.513279), glm::vec2(0.8125, 0.8125) });
	verts.push_back({ glm::vec3(-0.108386, 0.83147, -0.544895), glm::vec2(0.78125, 0.8125) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, -0.69352), glm::vec2(0.78125, 0.75) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, -0.653281), glm::vec2(0.8125, 0.25) });
	verts.push_back({ glm::vec3(-0.318189, -0.55557, -0.768177), glm::vec2(0.8125, 0.3125) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, -0.815493), glm::vec2(0.78125, 0.3125) });
	verts.push_back({ glm::vec3(-0.270598, 0.707107, -0.653281), glm::vec2(0.8125, 0.75) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, -0.69352), glm::vec2(0.78125, 0.75) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, -0.815493), glm::vec2(0.78125, 0.6875) });
	verts.push_back({ glm::vec3(-0.212607, -0.83147, -0.513279), glm::vec2(0.8125, 0.1875) });
	verts.push_back({ glm::vec3(-0.270598, -0.707107, -0.653281), glm::vec2(0.8125, 0.25) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, -0.69352), glm::vec2(0.78125, 0.25) });
	verts.push_back({ glm::vec3(-0.318189, 0.55557, -0.768177), glm::vec2(0.8125, 0.6875) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, -0.815493), glm::vec2(0.78125, 0.6875) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, -0.906127), glm::vec2(0.78125, 0.625) });
	verts.push_back({ glm::vec3(-0.212607, -0.83147, -0.513279), glm::vec2(0.8125, 0.1875) });
	verts.push_back({ glm::vec3(-0.108386, -0.83147, -0.544895), glm::vec2(0.78125, 0.1875) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, -0.37533), glm::vec2(0.78125, 0.125) });
	verts.push_back({ glm::vec3(-0.37533, 0.19509, -0.906127), glm::vec2(0.8125, 0.5625) });
	verts.push_back({ glm::vec3(-0.353554, 0.382683, -0.853553), glm::vec2(0.8125, 0.625) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, -0.906127), glm::vec2(0.78125, 0.625) });
	verts.push_back({ glm::vec3(-0.074658, -0.980785, -0.18024), glm::vec2(0.8125, 0.0625) });
	verts.push_back({ glm::vec3(-0.146447, -0.92388, -0.353553), glm::vec2(0.8125, 0.125) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, -0.37533), glm::vec2(0.78125, 0.125) });
	verts.push_back({ glm::vec3(-0.382683, 0, -0.923879), glm::vec2(0.8125, 0.5) });
	verts.push_back({ glm::vec3(-0.37533, 0.19509, -0.906127), glm::vec2(0.8125, 0.5625) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, -0.961939), glm::vec2(0.78125, 0.5625) });
	verts.push_back({ glm::vec3(-0.108386, -0.83147, -0.544895), glm::vec2(0.78125, 0.1875) });
	verts.push_back({ glm::vec3(-0.13795, -0.707107, -0.69352), glm::vec2(0.78125, 0.25) });
	verts.push_back({ glm::vec3(0, -0.707107, -0.707107), glm::vec2(0.75, 0.25) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, -0.906127), glm::vec2(0.78125, 0.625) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, -0.815493), glm::vec2(0.78125, 0.6875) });
	verts.push_back({ glm::vec3(0, 0.55557, -0.83147), glm::vec2(0.75, 0.6875) });
	verts.push_back({ glm::vec3(-0.108386, -0.83147, -0.544895), glm::vec2(0.78125, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.83147, -0.55557), glm::vec2(0.75, 0.1875) });
	verts.push_back({ glm::vec3(0, -0.92388, -0.382683), glm::vec2(0.75, 0.125) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, -0.961939), glm::vec2(0.78125, 0.5625) });
	verts.push_back({ glm::vec3(-0.18024, 0.382683, -0.906127), glm::vec2(0.78125, 0.625) });
	verts.push_back({ glm::vec3(0, 0.382683, -0.923879), glm::vec2(0.75, 0.625) });
	verts.push_back({ glm::vec3(-0.03806, -0.980785, -0.191342), glm::vec2(0.78125, 0.0625) });
	verts.push_back({ glm::vec3(-0.074658, -0.92388, -0.37533), glm::vec2(0.78125, 0.125) });
	verts.push_back({ glm::vec3(0, -0.92388, -0.382683), glm::vec2(0.75, 0.125) });
	verts.push_back({ glm::vec3(-0.19509, 0, -0.980784), glm::vec2(0.78125, 0.5) });
	verts.push_back({ glm::vec3(-0.191342, 0.19509, -0.961939), glm::vec2(0.78125, 0.5625) });
	verts.push_back({ glm::vec3(0, 0.19509, -0.980785), glm::vec2(0.75, 0.5625) });
	verts.push_back({ glm::vec3(-0.19509, 0, -0.980784), glm::vec2(0.78125, 0.5) });
	verts.push_back({ glm::vec3(0, 0, -1), glm::vec2(0.75, 0.5) });
	verts.push_back({ glm::vec3(0, -0.19509, -0.980785), glm::vec2(0.75, 0.4375) });
	verts.push_back({ glm::vec3(-0.03806, 0.980785, -0.191342), glm::vec2(0.78125, 0.9375) });
	verts.push_back({ glm::vec3(0, 0.980785, -0.19509), glm::vec2(0.75, 0.9375) });
	verts.push_back({ glm::vec3(0, 0.92388, -0.382683), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(-0.191342, -0.19509, -0.961939), glm::vec2(0.78125, 0.4375) });
	verts.push_back({ glm::vec3(0, -0.19509, -0.980785), glm::vec2(0.75, 0.4375) });
	verts.push_back({ glm::vec3(0, -0.382683, -0.923879), glm::vec2(0.75, 0.375) });
	verts.push_back({ glm::vec3(-0.108386, 0.83147, -0.544895), glm::vec2(0.78125, 0.8125) });
	verts.push_back({ glm::vec3(-0.074658, 0.92388, -0.37533), glm::vec2(0.78125, 0.875) });
	verts.push_back({ glm::vec3(0, 0.92388, -0.382683), glm::vec2(0.75, 0.875) });
	verts.push_back({ glm::vec3(-0.18024, -0.382683, -0.906127), glm::vec2(0.78125, 0.375) });
	verts.push_back({ glm::vec3(0, -0.382683, -0.923879), glm::vec2(0.75, 0.375) });
	verts.push_back({ glm::vec3(0, -0.55557, -0.83147), glm::vec2(0.75, 0.3125) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, -0.69352), glm::vec2(0.78125, 0.75) });
	verts.push_back({ glm::vec3(-0.108386, 0.83147, -0.544895), glm::vec2(0.78125, 0.8125) });
	verts.push_back({ glm::vec3(0, 0.83147, -0.55557), glm::vec2(0.75, 0.8125) });
	verts.push_back({ glm::vec3(-0.162212, -0.55557, -0.815493), glm::vec2(0.78125, 0.3125) });
	verts.push_back({ glm::vec3(0, -0.55557, -0.83147), glm::vec2(0.75, 0.3125) });
	verts.push_back({ glm::vec3(0, -0.707107, -0.707107), glm::vec2(0.75, 0.25) });
	verts.push_back({ glm::vec3(-0.162212, 0.55557, -0.815493), glm::vec2(0.78125, 0.6875) });
	verts.push_back({ glm::vec3(-0.13795, 0.707107, -0.69352), glm::vec2(0.78125, 0.75) });
	verts.push_back({ glm::vec3(0, 0.707107, -0.707107), glm::vec2(0.75, 0.75) });
	for (auto& v : verts)
		v.data[4] = 1 - v.data[4];
}
