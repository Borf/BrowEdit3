#define IMGUI_DEFINE_MATH_OPERATORS
#include <Windows.h>
#include "MapView.h"

#include <browedit/BrowEdit.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/Icons.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <browedit/shaders/SimpleShader.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/math/Polygon.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/TileSelectAction.h>
#include <browedit/actions/CubeHeightChangeAction.h>
#include <browedit/actions/CubeTileChangeAction.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <queue>

float TriangleHeight(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p)
{
	glm::vec3 v0 = b - a;
	glm::vec3 v1 = c - a;
	glm::vec3 v2 = p - a;
	const float denom = v0.x * v1.z - v1.x * v0.z;
	float v = (v2.x * v1.z - v1.x * v2.z) / denom;
	float w = (v0.x * v2.z - v2.x * v0.z) / denom;
	float u = 1.0f - v - w;
	return a.y * u + b.y * v + c.y * w;
}

bool TriangleContainsPoint(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& p)
{
	bool b1 = ((p.x - b.x) * (a.z - b.z) - (p.z - b.z) * (a.x - b.x)) <= 0.0f;
	bool b2 = ((p.x - c.x) * (b.z - c.z) - (p.z - c.z) * (b.x - c.x)) <= 0.0f;
	bool b3 = ((p.x - a.x) * (c.z - a.z) - (p.z - a.z) * (c.x - a.x)) <= 0.0f;
	return ((b1 == b2) && (b2 == b3));
}


void MapView::postRenderHeightMode(BrowEdit* browEdit)
{
	float gridSize = gridSizeTranslate;
	float gridOffset = gridOffsetTranslate;

	auto gnd = map->rootNode->getComponent<Gnd>();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();


	static bool splitTriangleFlip = false;
	static bool showCenterArrow = true;
	static bool showCornerArrows = true;
	static bool showEdgeArrows = true;
	static int edgeMode = 0;

	ImGui::Begin("Height Edit");
	ImGui::PushItemWidth(-200);

	if (ImGui::TreeNodeEx("Tool", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
	{
		if (browEdit->toolBarToggleButton("Rectangle", ICON_SELECT_RECTANGLE, browEdit->selectTool == BrowEdit::SelectTool::Rectangle, "Rectangle Select", ImVec4(1, 1, 1, 1)))
			browEdit->selectTool = BrowEdit::SelectTool::Rectangle;
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("Lasso", ICON_SELECT_LASSO, browEdit->selectTool == BrowEdit::SelectTool::Lasso, "Rectangle Select", ImVec4(1, 1, 1, 1)))
			browEdit->selectTool = BrowEdit::SelectTool::Lasso;
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("WandTex", ICON_SELECT_WAND_TEX, browEdit->selectTool == BrowEdit::SelectTool::WandTex, "Magic Wand (Texture)", ImVec4(1, 1, 1, 1)))
			browEdit->selectTool = BrowEdit::SelectTool::WandTex;
		ImGui::SameLine();
		if (browEdit->toolBarToggleButton("WandHeight", ICON_SELECT_WAND_HEIGHT, browEdit->selectTool == BrowEdit::SelectTool::WandHeight, "Magic Wand (Height)", ImVec4(1, 1, 1, 1)))
			browEdit->selectTool = BrowEdit::SelectTool::WandHeight;
		ImGui::TreePop();
	}

	if (browEdit->selectTool == BrowEdit::SelectTool::Rectangle || 
		browEdit->selectTool == BrowEdit::SelectTool::Lasso ||
		browEdit->selectTool == BrowEdit::SelectTool::WandTex ||
		browEdit->selectTool == BrowEdit::SelectTool::WandHeight)
	{
		if (ImGui::TreeNodeEx("Selection Options", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
		{
			browEdit->toolBarToggleButton("TriangleSplit", splitTriangleFlip ? ICON_HEIGHT_SPLIT_1 : ICON_HEIGHT_SPLIT_2, &splitTriangleFlip, "Change the way quads are split into triangles");
			ImGui::SameLine();
			browEdit->toolBarToggleButton("CenterArrow", ICON_HEIGHT_SELECT_CENTER, &showCenterArrow, "Show Center Arrow");
			ImGui::SameLine();
			browEdit->toolBarToggleButton("CornerArrow", ICON_HEIGHT_SELECT_CORNERS, &showCornerArrows, "Show Corner Arrows");
			ImGui::SameLine();
			browEdit->toolBarToggleButton("EdgeArrow", ICON_HEIGHT_SELECT_SIDES, &showEdgeArrows, "Show Edge Arrows");


			if (ImGui::TreeNodeEx("Edge handling", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
			{
				ImGui::RadioButton("Nothing", &edgeMode, 0);
				ImGui::SameLine();
				ImGui::RadioButton("Raise ground", &edgeMode, 1);

				ImGui::RadioButton("Build walls", &edgeMode, 3);
				ImGui::SameLine();
				ImGui::RadioButton("Snap ground", &edgeMode, 2);
				ImGui::TreePop();
			}

			if (ImGui::Button("Grow selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				map->growTileSelection(browEdit);
			if (ImGui::Button("Shrink selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				map->shrinkTileSelection(browEdit);

			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Actions", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
		{
			if (ImGui::Button("Smooth selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->smoothTiles(map, browEdit, map->tileSelection, 3);
			if (ImGui::Button("Smooth selection Horizontal", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->smoothTiles(map, browEdit, map->tileSelection, 1);
			if (ImGui::Button("Smooth selection Vertical", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->smoothTiles(map, browEdit, map->tileSelection, 2);
			if (ImGui::Button("Flatten selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->flattenTiles(map, browEdit, map->tileSelection);
			if (ImGui::Button("Connect tiles high", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->connectHigh(map, browEdit, map->tileSelection);
			if (ImGui::Button("Connect tiles low", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->connectLow(map, browEdit, map->tileSelection);

			if(ImGui::TreeNodeEx("Set Height", ImGuiTreeNodeFlags_Framed))
			{
				static float height = 0;
				ImGui::InputFloat("Height", &height);
				if (ImGui::Button("Set selection to height"))
				{
					auto action = new CubeHeightChangeAction(gnd, map->tileSelection);
					for (auto t : map->tileSelection)
						for(int i = 0; i < 4; i++)
							gnd->cubes[t.x][t.y]->heights[i] = height;
		
					action->setNewHeights(gnd, map->tileSelection);
					map->doAction(action, browEdit);
				}
				ImGui::TreePop();
			}

			//TODO
			/*if (ImGui::Button("Remove walls from selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
			{
			}
			if (ImGui::Button("Add walls to selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
			{
			}*/

			if (ImGui::TreeNodeEx("Random Generation", ImGuiTreeNodeFlags_Framed))
			{
				static float maxHeight = 1;
				static float minHeight = 0;
				static int raiseMode = 1;
				static bool edges = false;
				ImGui::SliderFloat("Min Random Value", &minHeight, -100, maxHeight);
				ImGui::SliderFloat("Max Random Value", &maxHeight, minHeight, 100);
				ImGui::Combo("Connect tiles", &raiseMode, "none\0high\0low\0");
				ImGui::Checkbox("Connect tiles around selection", &edges);
				if (ImGui::Button("Add random values to selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				{
					map->beginGroupAction("Random Tile Generation");
					gnd->addRandomHeight(map, browEdit, map->tileSelection, minHeight, maxHeight);
					if (raiseMode == 1)
						gnd->connectHigh(map, browEdit, map->tileSelection);
					else if (raiseMode == 2)
						gnd->connectLow(map, browEdit, map->tileSelection);
					if (edges)
					{
						if (raiseMode == 1)
							gnd->connectHigh(map, browEdit, map->getSelectionAroundTiles());
						else if (raiseMode == 2)
							gnd->connectLow(map, browEdit, map->getSelectionAroundTiles());
					}
					map->endGroupAction(browEdit);

				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNodeEx("Perlin Noise Generation", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
			{
				if (ImGui::Button("Add perlin noise", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
					gnd->perlinNoise(map->tileSelection);
				ImGui::TreePop();
			}


			if (ImGui::Button("Finish my map with AI", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				std::cout << "Coming soon®" << std::endl;
			ImGui::TreePop();
		}
	}

	if (map->tileSelection.size() == 1)
	{
		auto cube = gnd->cubes[map->tileSelection[0].x][map->tileSelection[0].y];
		if (ImGui::TreeNodeEx("Tile Details", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
		{
			bool changed = false;
			changed |= util::DragFloat(browEdit, map, map->rootNode, "H1", &cube->h1);
			changed |= util::DragFloat(browEdit, map, map->rootNode, "H2", &cube->h2);
			changed |= util::DragFloat(browEdit, map, map->rootNode, "H3", &cube->h3);
			changed |= util::DragFloat(browEdit, map, map->rootNode, "H4", &cube->h4);

			static const char* tileNames[] = {"Tile Up", "Tile Front", "Tile Side"};
			for (int t = 0; t < 3; t++)
			{
				ImGui::PushID(t);
				changed |= util::DragInt(browEdit, map, map->rootNode, tileNames[t], &cube->tileIds[t], 1.0f, 0, (int)gnd->tiles.size()-1);
				if (cube->tileIds[t] >= 0)
				{
					if (ImGui::TreeNodeEx(("Edit " + std::string(tileNames[t])).c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
					{
						auto tile = gnd->tiles[cube->tileIds[t]];
						int lightmapId = tile->lightmapIndex;
						if (changed |= util::DragInt(browEdit, map, map->rootNode, "LightmapID", &lightmapId, 1.0, 0, (int)gnd->lightmaps.size() - 1))
							tile->lightmapIndex = lightmapId;

						int texIndex = tile->textureIndex;
						if (changed |= util::DragInt(browEdit, map, map->rootNode, "TextureID", &texIndex, 1.0f, 0, (int)gnd->textures.size()-1))
							tile->textureIndex = texIndex;
						glm::vec4 color = glm::vec4(tile->color) / 255.0f;
						if (changed |= util::ColorEdit4(browEdit, map, map->rootNode, "Color", &color))
						{
							tile->color = glm::ivec4(color * 255.0f);
							for (auto tt : map->tileSelection)
								if (gnd->cubes[tt.x][tt.y]->tileIds[t] != -1)
									gnd->tiles[gnd->cubes[tt.x][tt.y]->tileIds[t]]->color = tile->color;
						}
						changed |= util::DragFloat2(browEdit, map, map->rootNode, "UV1", &tile->v1, 0.01f, 0.0f, 1.0f);
						changed |= util::DragFloat2(browEdit, map, map->rootNode, "UV2", &tile->v2, 0.01f, 0.0f, 1.0f);
						changed |= util::DragFloat2(browEdit, map, map->rootNode, "UV3", &tile->v3, 0.01f, 0.0f, 1.0f);
						changed |= util::DragFloat2(browEdit, map, map->rootNode, "UV4", &tile->v4, 0.01f, 0.0f, 1.0f);
						{ //UV editor
							ImGuiWindow* window = ImGui::GetCurrentWindow();
							const ImGuiStyle& style = ImGui::GetStyle();
							const ImGuiIO& IO = ImGui::GetIO();
							ImDrawList* DrawList = ImGui::GetWindowDrawList();

							ImGui::Text("Texture Edit");
							const float avail = ImGui::GetContentRegionAvailWidth();
							const float dim = ImMin(avail, 300.0f);
							ImVec2 Canvas(dim, dim);

							ImRect bb(window->DC.CursorPos, window->DC.CursorPos + Canvas);
							ImGui::ItemSize(bb);
							ImGui::ItemAdd(bb, NULL);
							const ImGuiID id = window->GetID("Texture Edit");
							//hovered |= 0 != ImGui::IsItemHovered(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id);
							ImGuiContext& g = *GImGui;

							ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
							window->DrawList->AddImage((ImTextureID)(long long)gndRenderer->textures[tile->textureIndex]->id(), bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), ImVec2(0, 1), ImVec2(1, 0));

							for (int i = 0; i < 4; i++)
							{
								int ii = (i + 1) % 4;
								window->DrawList->AddLine(
									ImVec2(bb.Min.x + tile->texCoords[i].x * bb.GetWidth(), bb.Max.y - tile->texCoords[i].y * bb.GetHeight()),
									ImVec2(bb.Min.x + tile->texCoords[ii].x * bb.GetWidth(), bb.Max.y - tile->texCoords[ii].y * bb.GetHeight()), ImGui::GetColorU32(ImGuiCol_Text, 1), 2.0f);
							}
							ImVec2 cursorPos = ImGui::GetCursorScreenPos();
							int i = 0;
							static bool c = false;
							static bool dragged = false;
							for (int i = 0; i < 4; i++)
							{
								ImVec2 pos = ImVec2(bb.Min.x + tile->texCoords[i].x * bb.GetWidth(), bb.Max.y - tile->texCoords[i].y * bb.GetHeight());
								window->DrawList->AddCircle(pos, 5, ImGui::GetColorU32(ImGuiCol_Text, 1), 0, 2.0f);
								ImGui::PushID(i);
								ImGui::SetCursorScreenPos(pos - ImVec2(5, 5));
								ImGui::InvisibleButton("button", ImVec2(2 * 5, 2 * 5));
								if (ImGui::IsItemActive() || ImGui::IsItemHovered())
									ImGui::SetTooltip("(%4.3f, %4.3f)", tile->texCoords[i].x, tile->texCoords[i].y);
								if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
								{
									tile->texCoords[i].x = (ImGui::GetIO().MousePos.x - bb.Min.x) / Canvas.x;
									tile->texCoords[i].y = 1 - (ImGui::GetIO().MousePos.y - bb.Min.y) / Canvas.y;
									bool snap = snapToGrid;
									if (ImGui::GetIO().KeyShift)
										snap = !snap;

									if (snap)
									{
										tile->texCoords[i] = glm::round(tile->texCoords[i] / gridSizeTranslate) * gridSizeTranslate;
									}
									gndRenderer->setChunkDirty(map->tileSelection[0].x, map->tileSelection[0].y);
									tile->texCoords[i] = glm::clamp(tile->texCoords[i], 0.0f, 1.0f);
									dragged = true;
								}
								else if (ImGui::IsMouseReleased(0) && dragged)
								{
									dragged = false;
									changed = true;
								}
								ImGui::PopID();
							}
							ImGui::SetCursorScreenPos(cursorPos);

						}
						if (lightmapId != -1)
						{
							glm::vec2 lm1((tile->lightmapIndex % GndRenderer::shadowmapRowCount) * (8.0f / GndRenderer::shadowmapSize) + 1.0f / GndRenderer::shadowmapSize, (tile->lightmapIndex / GndRenderer::shadowmapRowCount) * (8.0f / GndRenderer::shadowmapSize) + 1.0f / GndRenderer::shadowmapSize);
							glm::vec2 lm2(lm1 + glm::vec2(6.0f / GndRenderer::shadowmapSize, 6.0f / GndRenderer::shadowmapSize));
							ImGuiWindow* window = ImGui::GetCurrentWindow();
							ImGui::Text("Shadow");
							const ImGuiStyle& style = ImGui::GetStyle();
							const float avail = ImGui::GetContentRegionAvailWidth();
							const float dim = ImMin(avail, 300.0f);
							ImVec2 Canvas(dim, dim);
							ImRect bb(window->DC.CursorPos, window->DC.CursorPos + Canvas);
							ImGui::ItemSize(bb);
							ImGui::ItemAdd(bb, NULL);
							ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
							window->DrawList->AddImage((ImTextureID)(long long)gndRenderer->gndShadow->id(), bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), ImVec2(lm1.x, lm2.y), ImVec2(lm2.x, lm1.y));
						}

						ImGui::TreePop();
					}
				}
				ImGui::PopID();
			}

			if (changed)
				gndRenderer->setChunkDirty(map->tileSelection[0].x, map->tileSelection[0].y);
			ImGui::TreePop();
		}
	}
	ImGui::PopItemWidth();
	ImGui::End();

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
	glDepthMask(0);
	bool canSelect = true;


	auto mouse3D = gnd->rayCast(mouseRay, viewEmptyTiles);
	glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));

	ImGui::Begin("Statusbar");
	ImGui::SetNextItemWidth(100.0f);
	ImGui::InputInt2("Cursor:", glm::value_ptr(tileHovered));
	ImGui::SameLine();
	ImGui::End();

	bool snap = snapToGrid;
	if (ImGui::GetIO().KeyShift)
		snap = !snap;

	//draw paste
	if (browEdit->newCubes.size() > 0)
	{
		canSelect = false;
		std::vector<VertexP3T2N3> verts;
		float dist = 0.002f * cameraDistance;
		for (auto cube : browEdit->newCubes)
		{
			verts.push_back(VertexP3T2N3(glm::vec3(10 * (tileHovered.x + cube->pos.x), -cube->h3 + dist, 10 * gnd->height - 10 * (tileHovered.y + cube->pos.y)), glm::vec2(0), cube->normals[2]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * (tileHovered.x + cube->pos.x) + 10, -cube->h2 + dist, 10 * gnd->height - 10 * (tileHovered.y + cube->pos.y) + 10), glm::vec2(0), cube->normals[1]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * (tileHovered.x + cube->pos.x) + 10, -cube->h4 + dist, 10 * gnd->height - 10 * (tileHovered.y + cube->pos.y)), glm::vec2(0), cube->normals[3]));

			verts.push_back(VertexP3T2N3(glm::vec3(10 * (tileHovered.x + cube->pos.x), -cube->h1 + dist, 10 * gnd->height - 10 * (tileHovered.y + cube->pos.y) + 10), glm::vec2(0), cube->normals[0]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * (tileHovered.x + cube->pos.x), -cube->h3 + dist, 10 * gnd->height - 10 * (tileHovered.y + cube->pos.y)), glm::vec2(0), cube->normals[2]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * (tileHovered.x + cube->pos.x) + 10, -cube->h2 + dist, 10 * gnd->height - 10 * (tileHovered.y + cube->pos.y) + 10), glm::vec2(0), cube->normals[1]));
		}
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4); //TODO: vao
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);

		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0, 1, 0, 0.25f));
		glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());


		if (ImGui::IsMouseReleased(0))
		{
			std::vector<glm::ivec2> cubeSelected;
			for (auto c : browEdit->newCubes)
				cubeSelected.push_back(c->pos + tileHovered);
			auto action1 = new CubeHeightChangeAction(gnd, cubeSelected);
			auto action2 = new CubeTileChangeAction(gnd, cubeSelected);
			for (auto cube : browEdit->newCubes)
			{
				for (int i = 0; i < 4; i++)
					gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->heights[i] = cube->heights[i];
				for (int i = 0; i < 3; i++)
					gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->tileIds[i] = cube->tileIds[i];
				for (int i = 0; i < 4; i++)
					gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->normals[i] = cube->normals[i];
				gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->normal = cube->normal;
			}
			action1->setNewHeights(gnd, cubeSelected);
			action2->setNewTiles(gnd, cubeSelected);

			auto ga = new GroupAction();
			ga->addAction(action1);
			ga->addAction(action2);
			ga->addAction(new TileSelectAction(map, cubeSelected));
			map->doAction(ga, browEdit);
			for (auto c : browEdit->newCubes)
				delete c;
			browEdit->newCubes.clear();
		}


	}

	//draw selection
	if (map->tileSelection.size() > 0 && canSelect)
	{
		std::vector<VertexP3T2N3> verts;
		float dist = 0.002f * cameraDistance;
		for (auto& tile : map->tileSelection)
		{
			auto cube = gnd->cubes[tile.x][tile.y];
			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, -cube->h3 + dist, 10 * gnd->height - 10 * tile.y), glm::vec2(0), cube->normals[2]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0), cube->normals[1]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * tile.y), glm::vec2(0), cube->normals[3]));

			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, -cube->h1 + dist, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0), cube->normals[0]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, -cube->h3 + dist, 10 * gnd->height - 10 * tile.y), glm::vec2(0), cube->normals[2]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0), cube->normals[1]));
		}
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4); //TODO: vao
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);

		glLineWidth(1.0f);
		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0, 0, 1.0f));
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0, 0, 0.25f));
		glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());


		Gadget::setMatrices(nodeRenderContext.projectionMatrix, nodeRenderContext.viewMatrix);
		glDepthMask(1);

		glm::ivec2 maxValues(-99999, -99999), minValues(999999, 9999999);
		for (auto& s : map->tileSelection)
		{
			maxValues = glm::max(maxValues, s);
			minValues = glm::min(minValues, s);
		}
		static std::map<Gnd::Cube*, float[4]> originalValues;
		static glm::vec3 originalCorners[4];

		glm::vec3 pos[9];
		pos[0] = gnd->getPos(minValues.x, minValues.y, 0);
		pos[1] = gnd->getPos(maxValues.x, minValues.y, 1);
		pos[2] = gnd->getPos(minValues.x, maxValues.y, 2);
		pos[3] = gnd->getPos(maxValues.x, maxValues.y, 3);
		pos[4] = (pos[0] + pos[1] + pos[2] + pos[3]) / 4.0f;
		pos[5] = (pos[0] + pos[1]) / 2.0f;
		pos[6] = (pos[2] + pos[3]) / 2.0f;
		pos[7] = (pos[0] + pos[2]) / 2.0f;
		pos[8] = (pos[1] + pos[3]) / 2.0f;

		ImGui::Begin("Height Edit");
		if (ImGui::CollapsingHeader("Selection Details", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat("Corner 1", &pos[0].y);
			ImGui::DragFloat("Corner 2", &pos[1].y);
			ImGui::DragFloat("Corner 3", &pos[2].y);
			ImGui::DragFloat("Corner 4", &pos[3].y);
		}
		ImGui::End();


		static int dragIndex = -1;
		for (int i = 0; i < 9; i++)
		{
			if (!showCornerArrows && i < 4)
				continue;
			if (!showCenterArrow && i == 4)
				continue;
			if (!showEdgeArrows && i > 4)
				continue;
			glm::mat4 mat = glm::translate(glm::mat4(1.0f), pos[i]);
			if (maxValues.x - minValues.x <= 1 || maxValues.y - minValues.y <= 1)
				mat = glm::scale(mat, glm::vec3(0.25f, 0.25f, 0.25f));

			if (dragIndex == -1)
				gadgetHeight[i].disabled = false;
			else
				gadgetHeight[i].disabled = dragIndex != i;

			gadgetHeight[i].draw(mouseRay, mat);

			if (gadgetHeight[i].axisClicked)
			{
				dragIndex = i;
				mouseDragPlane.normal = glm::normalize(glm::vec3(nodeRenderContext.viewMatrix * glm::vec4(0, 0, 1, 1)) - glm::vec3(nodeRenderContext.viewMatrix * glm::vec4(0, 0, 0, 1)));
				mouseDragPlane.normal.y = 0;
				mouseDragPlane.normal = glm::normalize(mouseDragPlane.normal);
				mouseDragPlane.D = -glm::dot(pos[i], mouseDragPlane.normal);

				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				mouseDragStart = mouseRay.origin + f * mouseRay.dir;

				originalValues.clear();
				for (auto& t : map->tileSelection)
					for (int ii = 0; ii < 4; ii++)
						originalValues[gnd->cubes[t.x][t.y]][ii] = gnd->cubes[t.x][t.y]->heights[ii];

				for (int ii = 0; ii < 4; ii++)
					originalCorners[ii] = pos[ii];

				canSelect = false;
			}
			else if (gadgetHeight[i].axisReleased)
			{
				dragIndex = -1;
				canSelect = false;

				std::map<Gnd::Cube*, float[4]> newValues;
				for (auto& t : originalValues)
					for (int ii = 0; ii < 4; ii++)
						newValues[t.first][ii] = t.first->heights[ii];
				map->doAction(new CubeHeightChangeAction(originalValues, newValues), browEdit);
			}
			else if (gadgetHeight[i].axisDragged)
			{

				if (snap)
				{
					glm::mat4 mat(1.0f);
					
					glm::vec3 rounded = pos[i];
					if (!gridLocal)
						rounded.y = glm::floor(rounded.y / gridSize) * gridSize;

					mat = glm::translate(mat, rounded);
					mat = glm::rotate(mat, glm::radians(90.0f), glm::vec3(1, 0, 0));
					simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, mat);
					simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0, 0, 0, 1));
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
					gridVbo->unBind();
				}


				float snapHeight = 0;
				if (ImGui::GetIO().KeyCtrl)
				{
					if (tileHovered.x >= 0 && tileHovered.x < gnd->width && tileHovered.y >= 0 && tileHovered.y < gnd->height)
					{
						glm::vec2 tileHoveredOffset((mouse3D.x / 10) - tileHovered.x, tileHovered.y - (gnd->height - mouse3D.z / 10));

						ImGui::Begin("Statusbar");
						ImGui::InputFloat2("Offset", glm::value_ptr(tileHoveredOffset));
						ImGui::End();

						int index = 0;
						if (tileHoveredOffset.x > 0.5 && tileHoveredOffset.y > 0.5)
							index = 0;
						if (tileHoveredOffset.x < 0.5 && tileHoveredOffset.y > 0.5)
							index = 1;
						if (tileHoveredOffset.x > 0.5 && tileHoveredOffset.y < 0.5)
							index = 2;
						if (tileHoveredOffset.x < 0.5 && tileHoveredOffset.y < 0.5)
							index = 3;

						snapHeight = gnd->cubes[tileHovered.x][tileHovered.y]->heights[index];
						glUseProgram(0);
						glPointSize(10.0);
						glBegin(GL_POINTS);
						glVertex3f(	tileHovered.x * 10.0f + ((index % 2) == 0 ? 10.0f : 0.0f), 
									-snapHeight+1, 
									(gnd->height-tileHovered.y) * 10.0f + ((index / 2) == 0 ? 10.0f : 0.0f));
						glEnd();
					}
				}


				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				glm::vec3 mouseOffset = (mouseRay.origin + f * mouseRay.dir) - mouseDragStart;
				if (snap && gridLocal)
					mouseOffset = glm::round(mouseOffset / (float)gridSize) * (float)gridSize;

				canSelect = false;
				static int masks[] = { 0b0001, 0b0010, 0b0100, 0b1000, 0b1111, 0b0011, 0b1100, 0b0101, 0b1010 };
				int mask = masks[i];

				for (int ii = 0; ii < 4; ii++)
				{
					pos[ii] = originalCorners[ii];
					if (((mask >> ii) & 1) != 0)
					{
						pos[ii].y -= mouseOffset.y;
						if (snap && !gridLocal)
						{
							pos[ii].y += 2*mouseOffset.y;
							pos[ii].y = glm::round(pos[ii].y / gridSize) * gridSize;
							pos[ii].y = originalCorners[ii].y + (originalCorners[ii].y - pos[ii].y);
						}
						if (ImGui::GetIO().KeyCtrl)
						{
							pos[ii].y = originalCorners[ii].y + (originalCorners[ii].y + snapHeight);
						}
					}
				}

				std::vector<glm::ivec2> tilesAround;
				auto isTileSelected = [&](int x, int y) { return std::find(map->tileSelection.begin(), map->tileSelection.end(), glm::ivec2(x, y)) != map->tileSelection.end(); };

				for (auto& t : map->tileSelection)
				{
					if (gndRenderer)
						gndRenderer->setChunkDirty(t.x, t.y);
					for (int ii = 0; ii < 4; ii++)
					{
						// 0 1
						// 2 3
						glm::vec3 p = gnd->getPos(t.x, t.y, ii);
						if (!splitTriangleFlip)	// /
						{
							if (TriangleContainsPoint(pos[0], pos[1], pos[2], p))
								gnd->cubes[t.x][t.y]->heights[ii] = originalValues[gnd->cubes[t.x][t.y]][ii] + (TriangleHeight(pos[0], pos[1], pos[2], p) - TriangleHeight(originalCorners[0], originalCorners[1], originalCorners[2], p));
							if (TriangleContainsPoint(pos[1], pos[3], pos[2], p))
								gnd->cubes[t.x][t.y]->heights[ii] = originalValues[gnd->cubes[t.x][t.y]][ii] + (TriangleHeight(pos[1], pos[3], pos[2], p) - TriangleHeight(originalCorners[1], originalCorners[3], originalCorners[2], p));
						}
						else // \ 
						{
							if (TriangleContainsPoint(pos[0], pos[3], pos[2], p))
								gnd->cubes[t.x][t.y]->heights[ii] = originalValues[gnd->cubes[t.x][t.y]][ii] + (TriangleHeight(pos[0], pos[3], pos[2], p) - TriangleHeight(originalCorners[0], originalCorners[3], originalCorners[2], p));
							if (TriangleContainsPoint(pos[0], pos[1], pos[3], p))
								gnd->cubes[t.x][t.y]->heights[ii] = originalValues[gnd->cubes[t.x][t.y]][ii] + (TriangleHeight(pos[0], pos[1], pos[3], p) - TriangleHeight(originalCorners[0], originalCorners[1], originalCorners[3], p));
						}
						if (edgeMode == 1 || edgeMode == 2) //smooth raise area around these tiles
						{
							for (int xx = -1; xx <= 1; xx++)
								for (int yy = -1; yy <= 1; yy++)
									if (t.x + xx >= 0 && t.x + xx < gnd->width && t.y + yy >= 0 && t.y + yy < gnd->height &&
										std::find(tilesAround.begin(), tilesAround.end(), glm::ivec2(t.x + xx, t.y + yy)) == tilesAround.end() &&
										!isTileSelected(t.x+xx, t.y+yy))
										tilesAround.push_back(glm::ivec2(t.x + xx, t.y + yy));
						}
					}
					gnd->cubes[t.x][t.y]->calcNormal();
				}

				auto getPointHeight = [&](int x1, int y1, int i1, int x2, int y2, int i2, int x3, int y3, int i3, float def)
				{
					if (x1 >= 0 && x1 < gnd->width && y1 >= 0 && y1 < gnd->height && isTileSelected(x1, y1))
						return gnd->cubes[x1][y1]->heights[i1];
					if (x2 >= 0 && x2 < gnd->width && y2 >= 0 && y2 < gnd->height && isTileSelected(x2, y2))
						return gnd->cubes[x2][y2]->heights[i2];
					if (x3 >= 0 && x3 < gnd->width && y3 >= 0 && y3 < gnd->height && isTileSelected(x3, y3))
						return gnd->cubes[x3][y3]->heights[i3];
					return def;
				};
				auto getPointHeightDiff = [&](int x1, int y1, int i1, int x2, int y2, int i2, int x3, int y3, int i3)
				{
					if (x1 >= 0 && x1 < gnd->width && y1 >= 0 && y1 < gnd->height && isTileSelected(x1, y1))
						return originalValues[gnd->cubes[x1][y1]][i1] - gnd->cubes[x1][y1]->heights[i1];
					if (x2 >= 0 && x2 < gnd->width && y2 >= 0 && y2 < gnd->height && isTileSelected(x2, y2))
						return originalValues[gnd->cubes[x2][y2]][i2] - gnd->cubes[x2][y2]->heights[i2];
					if (x3 >= 0 && x3 < gnd->width && y3 >= 0 && y3 < gnd->height && isTileSelected(x3, y3))
						return originalValues[gnd->cubes[x3][y3]][i3] - gnd->cubes[x3][y3]->heights[i3];
					return 0.0f;
				};

				for (auto a : tilesAround)
				{
					auto cube = gnd->cubes[a.x][a.y];
					if (originalValues.find(cube) == originalValues.end())
						for (int ii = 0; ii < 4; ii++)
							originalValues[cube][ii] = cube->heights[ii];
					//h1 bottomleft
					//h2 bottomright
					//h3 topleft
					//h4 topright
					if (edgeMode == 1) //raise ground
					{
						cube->heights[0] = originalValues[cube][0] - getPointHeightDiff(a.x - 1, a.y - 1, 3, a.x - 1, a.y, 1, a.x, a.y - 1, 2);
						cube->heights[1] = originalValues[cube][1] - getPointHeightDiff(a.x + 1, a.y - 1, 2, a.x + 1, a.y, 0, a.x, a.y - 1, 3);
						cube->heights[2] = originalValues[cube][2] - getPointHeightDiff(a.x - 1, a.y + 1, 1, a.x - 1, a.y, 3, a.x, a.y + 1, 0);
						cube->heights[3] = originalValues[cube][3] - getPointHeightDiff(a.x + 1, a.y + 1, 0, a.x + 1, a.y, 2, a.x, a.y + 1, 1);
					}
					else if (edgeMode == 2) //snap ground
					{
						cube->heights[0] = getPointHeight(a.x - 1, a.y - 1, 3, a.x - 1, a.y, 1, a.x, a.y - 1, 2, cube->heights[0]);
						cube->heights[1] = getPointHeight(a.x + 1, a.y - 1, 2, a.x + 1, a.y, 0, a.x, a.y - 1, 3, cube->heights[1]);
						cube->heights[2] = getPointHeight(a.x - 1, a.y + 1, 1, a.x - 1, a.y, 3, a.x, a.y + 1, 0, cube->heights[2]);
						cube->heights[3] = getPointHeight(a.x + 1, a.y + 1, 0, a.x + 1, a.y, 2, a.x, a.y + 1, 1, cube->heights[3]);
					}
					cube->calcNormal();
				}

				if (edgeMode == 3) // add walls
				{
					for (auto t : map->tileSelection)
					{
						//h1 bottomleft
						//h2 bottomright
						//h3 topleft
						//h4 topright
						if (t.x > 0 && !isTileSelected(t.x - 1, t.y) && (
							gnd->cubes[t.x][t.y]->h1 != gnd->cubes[t.x - 1][t.y]->h2 || 
							gnd->cubes[t.x][t.y]->h3 != gnd->cubes[t.x - 1][t.y]->h4) &&
							gnd->cubes[t.x - 1][t.y]->tileSide == -1)
							gnd->cubes[t.x - 1][t.y]->tileSide = 1;

						if (t.x < gnd->width-1 && !isTileSelected(t.x + 1, t.y) && (
							gnd->cubes[t.x][t.y]->h2 != gnd->cubes[t.x + 1][t.y]->h1 ||
							gnd->cubes[t.x][t.y]->h4 != gnd->cubes[t.x + 1][t.y]->h3) &&
							gnd->cubes[t.x][t.y]->tileSide == -1)
							gnd->cubes[t.x][t.y]->tileSide = 1;

						if (t.y > 0 && !isTileSelected(t.x, t.y - 1) && (
							gnd->cubes[t.x][t.y]->h1 != gnd->cubes[t.x][t.y - 1]->h3 ||
							gnd->cubes[t.x][t.y]->h2 != gnd->cubes[t.x][t.y - 1]->h4) &&
							gnd->cubes[t.x][t.y - 1]->tileFront == -1)
							gnd->cubes[t.x][t.y - 1]->tileFront = 1;

						if (t.y < gnd->height-1 && !isTileSelected(t.x, t.y + 1) && (
							gnd->cubes[t.x][t.y]->h3 != gnd->cubes[t.x][t.y + 1]->h1 ||
							gnd->cubes[t.x][t.y]->h4 != gnd->cubes[t.x][t.y + 1]->h2) &&
							gnd->cubes[t.x][t.y]->tileFront == -1)
							gnd->cubes[t.x][t.y]->tileFront = 1;

					}
				}


				for (auto& t : map->tileSelection)
					gnd->cubes[t.x][t.y]->calcNormals(gnd, t.x, t.y);
				for (auto& t : tilesAround)
					gnd->cubes[t.x][t.y]->calcNormals(gnd, t.x, t.y);
			}
		}

		simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
		glDepthMask(0);
	}




	if (hovered && canSelect)
	{
		if (ImGui::IsMouseDown(0))
		{
			if (!mouseDown)
				mouseDragStart = mouse3D;
			mouseDown = true;
		}

		if (ImGui::IsMouseReleased(0))
		{
			auto mouseDragEnd = mouse3D;
			mouseDown = false;

			std::vector<glm::ivec2> newSelection;
			if (ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl)
				newSelection = map->tileSelection;

			if (browEdit->selectTool == BrowEdit::SelectTool::Rectangle)
			{
				int tileMinX = (int)glm::floor(glm::min(mouseDragStart.x, mouseDragEnd.x) / 10);
				int tileMaxX = (int)glm::ceil(glm::max(mouseDragStart.x, mouseDragEnd.x) / 10);

				int tileMaxY = gnd->height - (int)glm::floor(glm::min(mouseDragStart.z, mouseDragEnd.z) / 10) + 1;
				int tileMinY = gnd->height - (int)glm::ceil(glm::max(mouseDragStart.z, mouseDragEnd.z) / 10) + 1;

				if (tileMinX >= 0 && tileMaxX < gnd->width + 1 && tileMinY >= 0 && tileMaxY < gnd->height + 1)
					for (int x = tileMinX; x < tileMaxX; x++)
						for (int y = tileMinY; y < tileMaxY; y++)
							if (ImGui::GetIO().KeyCtrl)
							{
								if (std::find(newSelection.begin(), newSelection.end(), glm::ivec2(x, y)) != newSelection.end())
									newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [&](const glm::ivec2& el) { return el.x == x && el.y == y; }));
							}
							else if (std::find(newSelection.begin(), newSelection.end(), glm::ivec2(x, y)) == newSelection.end())
								newSelection.push_back(glm::ivec2(x, y));
			}
			else if (browEdit->selectTool == BrowEdit::SelectTool::Lasso)
			{
				math::Polygon polygon;
				for (size_t i = 0; i < selectLasso.size(); i++)
					polygon.push_back(glm::vec2(selectLasso[i]));
				for (int x = 0; x < gnd->width; x++)
				{
					for (int y = 0; y < gnd->height; y++)
					{
						if(polygon.contains(glm::vec2(x, y)))
							if (ImGui::GetIO().KeyCtrl)
							{
								if (std::find(newSelection.begin(), newSelection.end(), glm::ivec2(x, y)) != newSelection.end())
									newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [&](const glm::ivec2& el) { return el.x == x && el.y == y; }));
							}
							else if (std::find(newSelection.begin(), newSelection.end(), glm::ivec2(x, y)) == newSelection.end())
								newSelection.push_back(glm::ivec2(x, y));

					}
				}
				for (auto t : selectLasso)
					if (ImGui::GetIO().KeyCtrl)
					{
						if (std::find(newSelection.begin(), newSelection.end(), glm::ivec2(t.x, t.y)) != newSelection.end())
							newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [&](const glm::ivec2& el) { return el.x == t.x && el.y == t.y; }));
					}
					else if (std::find(newSelection.begin(), newSelection.end(), glm::ivec2(t.x, t.y)) == newSelection.end())
						newSelection.push_back(glm::ivec2(t.x, t.y));
				selectLasso.clear();
			}
			else if (browEdit->selectTool == BrowEdit::SelectTool::WandTex)
			{
				glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));

				std::vector<glm::ivec2> tilesToFill;
				std::queue<glm::ivec2> queue;
				std::map<int, bool> done;

				glm::ivec2 offsets[4] = { glm::ivec2(-1,0), glm::ivec2(1,0), glm::ivec2(0,-1), glm::ivec2(0,1) };
				queue.push(tileHovered);
				int c = 0;
				while (!queue.empty())
				{
					auto p = queue.front();
					queue.pop();
					for (const auto& o : offsets)
					{
						auto pp = p + o;
						if (!gnd->inMap(pp) || !gnd->inMap(p))
							continue;
						auto c = gnd->cubes[pp.x][pp.y];
						if (c->tileUp == -1 || gnd->cubes[p.x][p.y]->tileUp == -1)
							continue;
						auto t = gnd->tiles[c->tileUp];
						if (t->textureIndex != gnd->tiles[gnd->cubes[p.x][p.y]->tileUp]->textureIndex)
							continue;
						if (done.find(pp.x + gnd->width * pp.y) == done.end())
						{
							done[pp.x + gnd->width * pp.y] = true;
							queue.push(pp);
						}
					}
					if (gnd->inMap(p) && std::find(tilesToFill.begin(), tilesToFill.end(), p) == tilesToFill.end())
						tilesToFill.push_back(p);
					c++;
				}
				for (const auto& t : tilesToFill)
				{
					if (ImGui::GetIO().KeyCtrl)
					{
						if (std::find(newSelection.begin(), newSelection.end(), t) != newSelection.end())
							newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [&](const glm::ivec2& el) { return el.x == t.x && el.y == t.y; }));
					}
					else if (std::find(newSelection.begin(), newSelection.end(), t) == newSelection.end())
						newSelection.push_back(t);
				}
			}
			else if (browEdit->selectTool == BrowEdit::SelectTool::WandHeight)
			{
				glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));

				std::vector<glm::ivec2> tilesToFill;
				std::queue<glm::ivec2> queue;
				std::map<int, bool> done;

				glm::ivec2 offsets[4] = { glm::ivec2(-1,0), glm::ivec2(1,0), glm::ivec2(0,-1), glm::ivec2(0,1) };
				queue.push(tileHovered);
				int c = 0;
				while (!queue.empty())
				{
					auto p = queue.front();
					queue.pop();
					for (const auto& o : offsets)
					{
						auto pp = p + o;
						if (!gnd->inMap(pp) || !gnd->inMap(p))
							continue;
						auto c = gnd->cubes[pp.x][pp.y];

						if (!c->sameHeight(*gnd->cubes[p.x][p.y]))
							continue;
						if (done.find(pp.x + gnd->width * pp.y) == done.end())
						{
							done[pp.x + gnd->width * pp.y] = true;
							queue.push(pp);
						}
					}
					if (gnd->inMap(p) && std::find(tilesToFill.begin(), tilesToFill.end(), p) == tilesToFill.end())
						tilesToFill.push_back(p);
					c++;
				}
				for (const auto& t : tilesToFill)
				{
					if (ImGui::GetIO().KeyCtrl)
					{
						if (std::find(newSelection.begin(), newSelection.end(), t) != newSelection.end())
							newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [&](const glm::ivec2& el) { return el.x == t.x && el.y == t.y; }));
					}
					else if (std::find(newSelection.begin(), newSelection.end(), t) == newSelection.end())
						newSelection.push_back(t);
				}
			}


			if (map->tileSelection != newSelection)
			{
				map->doAction(new TileSelectAction(map, newSelection), browEdit);
			}
		}
	}
	if (mouseDown)
	{
		auto mouseDragEnd = mouse3D;
		std::vector<VertexP3T2N3> verts;
		float dist = 0.002f * cameraDistance;

		int tileX = (int)glm::floor(mouseDragEnd.x / 10);
		int tileY = gnd->height - (int)glm::floor(mouseDragEnd.z / 10);

		int tileMinX = (int)glm::floor(glm::min(mouseDragStart.x, mouseDragEnd.x) / 10);
		int tileMaxX = (int)glm::ceil(glm::max(mouseDragStart.x, mouseDragEnd.x) / 10);

		int tileMaxY = gnd->height - (int)glm::floor(glm::min(mouseDragStart.z, mouseDragEnd.z) / 10) + 1;
		int tileMinY = gnd->height - (int)glm::ceil(glm::max(mouseDragStart.z, mouseDragEnd.z) / 10) + 1;


		ImGui::Begin("Statusbar");
		ImGui::Text("Selection: (%d,%d) - (%d,%d)", tileMinX, tileMinY, tileMaxX, tileMaxY);
		ImGui::End();

		if (browEdit->selectTool == BrowEdit::SelectTool::Rectangle)
		{
			if (tileMinX >= 0 && tileMaxX < gnd->width + 1 && tileMinY >= 0 && tileMaxY < gnd->height + 1)
				for (int x = tileMinX; x < tileMaxX; x++)
				{
					for (int y = tileMinY; y < tileMaxY; y++)
					{
						auto cube = gnd->cubes[x][y];
						verts.push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h3 + dist, 10 * gnd->height - 10 * y), glm::vec2(0), cube->normals[2]));
						verts.push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h1 + dist, 10 * gnd->height - 10 * y + 10), glm::vec2(0), cube->normals[0]));
						verts.push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * y), glm::vec2(0), cube->normals[3]));

						verts.push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h1 + dist, 10 * gnd->height - 10 * y + 10), glm::vec2(0), cube->normals[0]));
						verts.push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * y), glm::vec2(0), cube->normals[3]));
						verts.push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * y + 10), glm::vec2(0), cube->normals[1]));
					}
				}
		}
		else if (browEdit->selectTool == BrowEdit::SelectTool::Lasso) // lasso
		{
			if (tileX >= 0 && tileX < gnd->width && tileY >= 0 && tileY < gnd->height)
			{
				if (selectLasso.empty())
					selectLasso.push_back(glm::ivec2(tileX, tileY));
				else
					if (selectLasso[selectLasso.size() - 1] != glm::ivec2(tileX, tileY))
						selectLasso.push_back(glm::ivec2(tileX, tileY));
			}

			for (auto& t : selectLasso)
			{
				auto cube = gnd->cubes[t.x][t.y];
				verts.push_back(VertexP3T2N3(glm::vec3(10 * t.x, -cube->h3 + dist, 10 * gnd->height - 10 * t.y), glm::vec2(0), cube->normals[2]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * t.x, -cube->h1 + dist, 10 * gnd->height - 10 * t.y + 10), glm::vec2(0), cube->normals[0]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * t.x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * t.y), glm::vec2(0), cube->normals[3]));

				verts.push_back(VertexP3T2N3(glm::vec3(10 * t.x, -cube->h1 + dist, 10 * gnd->height - 10 * t.y + 10), glm::vec2(0), cube->normals[0]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * t.x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * t.y), glm::vec2(0), cube->normals[3]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * t.x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * t.y + 10), glm::vec2(0), cube->normals[1]));
			}
		}

		if (verts.size() > 0)
		{
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
			glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0, 0, 0.5f));
			glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
		}


	}

	glDepthMask(1);






	fbo->unbind();
}