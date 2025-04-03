#define IMGUI_DEFINE_MATH_OPERATORS
#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <browedit/Map.h>
#include <browedit/MapView.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gat.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/GatRenderer.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/WaterRenderer.h>
#include <browedit/actions/CubeHeightChangeAction.h>
#include <browedit/gl/Texture.h>
#include <browedit/HotkeyRegistry.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <GLFW/glfw3.h>

GLint oldTextureId;

void BrowEdit::showHeightWindow()
{

	ImGui::Begin("Height Edit");
	if (!activeMapView)
	{
		ImGui::End();
		return;
	}
	auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();
	auto gndRenderer = activeMapView->map->rootNode->getComponent<GndRenderer>();
	ImGui::PushItemWidth(-200);

	if (ImGui::TreeNodeEx("Tool", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
	{
		toolBarToggleButton("Rectangle", ICON_SELECT_RECTANGLE, !heightDoodle && selectTool == BrowEdit::SelectTool::Rectangle, "Rectangle Select", HotkeyAction::HeightEdit_Rectangle, ImVec4(1, 1, 1, 1));
		ImGui::SameLine();
		toolBarToggleButton("Lasso", ICON_SELECT_LASSO, !heightDoodle && selectTool == BrowEdit::SelectTool::Lasso, "Lasso Select", HotkeyAction::HeightEdit_Lasso, ImVec4(1, 1, 1, 1));
		ImGui::SameLine();
		toolBarToggleButton("Doodle", ICON_HEIGHT_DOODLE, heightDoodle, "Height Doodling", HotkeyAction::HeightEdit_Doodle, ImVec4(1, 1, 1, 1));
		ImGui::SameLine();
		toolBarToggleButton("WandTex", ICON_SELECT_WAND_TEX, !heightDoodle && selectTool == BrowEdit::SelectTool::WandTex, "Magic Wand (Texture)", HotkeyAction::HeightEdit_MagicWandTexture, ImVec4(1, 1, 1, 1));
		ImGui::SameLine();
		toolBarToggleButton("WandHeight", ICON_SELECT_WAND_HEIGHT, !heightDoodle && selectTool == BrowEdit::SelectTool::WandHeight, "Magic Wand (Height)", HotkeyAction::HeightEdit_MagicWandHeight, ImVec4(1, 1, 1, 1));
		ImGui::SameLine();
		toolBarToggleButton("AllTex", ICON_SELECT_ALL_TEX, !heightDoodle && selectTool == BrowEdit::SelectTool::AllTex, "Select All (Texture)", HotkeyAction::HeightEdit_SelectAllTexture, ImVec4(1, 1, 1, 1));
		ImGui::SameLine();
		toolBarToggleButton("AllHeight", ICON_SELECT_ALL_HEIGHT, !heightDoodle && selectTool == BrowEdit::SelectTool::AllHeight, "Select All (Height)", HotkeyAction::HeightEdit_SelectAllHeight, ImVec4(1, 1, 1, 1));
		ImGui::TreePop();
	}

	if (!heightDoodle)
	{
		if (ImGui::TreeNodeEx("Selection Options", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
		{
			toolBarToggleButton("TriangleSplit", windowData.heightEdit.splitTriangleFlip ? ICON_HEIGHT_SPLIT_1 : ICON_HEIGHT_SPLIT_2, &windowData.heightEdit.splitTriangleFlip, "Change the way quads are split into triangles");
			ImGui::SameLine();
			toolBarToggleButton("CenterArrow", ICON_HEIGHT_SELECT_CENTER, &windowData.heightEdit.showCenterArrow, "Show Center Arrow");
			ImGui::SameLine();
			toolBarToggleButton("CornerArrow", ICON_HEIGHT_SELECT_CORNERS, &windowData.heightEdit.showCornerArrows, "Show Corner Arrows");
			ImGui::SameLine();
			toolBarToggleButton("EdgeArrow", ICON_HEIGHT_SELECT_SIDES, &windowData.heightEdit.showEdgeArrows, "Show Edge Arrows");


			if (ImGui::TreeNodeEx("Edge handling", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
			{
				ImGui::RadioButton("Nothing", &windowData.heightEdit.edgeMode, 0);
				ImGui::SameLine();
				ImGui::RadioButton("Raise ground", &windowData.heightEdit.edgeMode, 1);

				ImGui::RadioButton("Build walls", &windowData.heightEdit.edgeMode, 3);
				ImGui::SameLine();
				ImGui::RadioButton("Snap ground", &windowData.heightEdit.edgeMode, 2);
				ImGui::TreePop();
			}

			if (ImGui::Button("Grow selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				activeMapView->map->growTileSelection(this);
			if (ImGui::Button("Shrink selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				activeMapView->map->shrinkTileSelection(this);

			ImGui::TreePop();
		}
		if (ImGui::TreeNodeEx("Actions", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
		{
			ImGui::BeginGroup();
			if (ImGui::Button("Smooth selection", ImVec2(ImGui::GetContentRegionAvailWidth() - 12 - config.toolbarButtonSize, 0)))
				gnd->smoothTiles(activeMapView->map, this, activeMapView->map->tileSelection, 3);
			ImGui::SameLine();
			toolBarToggleButton("Connect", ICON_NOICON, false, "Connects tiles around selection");
			ImGui::EndGroup();
			if (ImGui::Button("Smooth selection Horizontal", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->smoothTiles(activeMapView->map, this, activeMapView->map->tileSelection, 1);
			if (ImGui::Button("Smooth selection Vertical", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->smoothTiles(activeMapView->map, this, activeMapView->map->tileSelection, 2);
			if (ImGui::Button("Flatten selection", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->flattenTiles(activeMapView->map, this, activeMapView->map->tileSelection);
			if (ImGui::Button("Connect tiles high", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->connectHigh(activeMapView->map, this, activeMapView->map->tileSelection);
			if (ImGui::Button("Connect tiles low", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				gnd->connectLow(activeMapView->map, this, activeMapView->map->tileSelection);

			if (ImGui::TreeNodeEx("Set Height", ImGuiTreeNodeFlags_Framed))
			{
				static float height = 0;
				ImGui::InputFloat("Height", &height);
				if (ImGui::Button("Set selection to height"))
				{
					auto action = new CubeHeightChangeAction<Gnd, Gnd::Cube>(gnd, activeMapView->map->tileSelection);
					for (auto t : activeMapView->map->tileSelection)
						for (int i = 0; i < 4; i++)
							gnd->cubes[t.x][t.y]->heights[i] = height;

					action->setNewHeights(gnd, activeMapView->map->tileSelection);
					activeMapView->map->doAction(action, this);
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
					activeMapView->map->beginGroupAction("Random Tile Generation");
					gnd->addRandomHeight(activeMapView->map, this, activeMapView->map->tileSelection, minHeight, maxHeight);
					if (raiseMode == 1)
						gnd->connectHigh(activeMapView->map, this, activeMapView->map->tileSelection);
					else if (raiseMode == 2)
						gnd->connectLow(activeMapView->map, this, activeMapView->map->tileSelection);
					if (edges)
					{
						if (raiseMode == 1)
							gnd->connectHigh(activeMapView->map, this, activeMapView->map->getSelectionAroundTiles());
						else if (raiseMode == 2)
							gnd->connectLow(activeMapView->map, this, activeMapView->map->getSelectionAroundTiles());
					}
					activeMapView->map->endGroupAction(this);

				}
				ImGui::TreePop();
			}
			if (ImGui::TreeNodeEx("Perlin Noise Generation", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
			{
				if (ImGui::Button("Add perlin noise", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
					gnd->perlinNoise(activeMapView->map->tileSelection);
				ImGui::TreePop();
			}

			if (ImGui::Button("Crop Map (NO UNDO)", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
			{
				windowData.cropWindowVisible = true;
			}

			if (ImGui::Button("Finish my map with AI", ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
				std::cout << "Coming soon®" << std::endl;
			ImGui::TreePop();
		}
	}
	else //heightDoodle
	{
		if (ImGui::TreeNodeEx("Brush Options", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragInt("Brush Size", &windowData.heightEdit.doodleSize, 1.0f, 0, 10);
			ImGui::DragFloat("Brush Hardness", &windowData.heightEdit.doodleHardness, 0.1f, 0.0f, 1.0f);
			ImGui::DragFloat("Brush Speed", &windowData.heightEdit.doodleSpeed, 0.01f, 0, 1);

			ImGui::TreePop();
			if (ImGui::IsKeyPressed(GLFW_KEY_KP_ADD))
				windowData.heightEdit.doodleSize++;
			if (ImGui::IsKeyPressed(GLFW_KEY_KP_SUBTRACT))
				windowData.heightEdit.doodleSize = glm::max(windowData.heightEdit.doodleSize - 1, 0);



		}
	}

	if (activeMapView->map->tileSelection.size() > 0 && !heightDoodle)
	{
		if (activeMapView->map->tileSelection.size() > 1)
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "WARNING, THESE SETTINGS ONLY\nWORK ON THE FIRST TILE");
		auto cube = gnd->cubes[activeMapView->map->tileSelection[0].x][activeMapView->map->tileSelection[0].y];
		if (ImGui::TreeNodeEx("Tile Details", ImGuiTreeNodeFlags_Framed))
		{
			bool changed = false;
			changed |= util::DragFloat(this, activeMapView->map, activeMapView->map->rootNode, "H1", &cube->h1);
			changed |= util::DragFloat(this, activeMapView->map, activeMapView->map->rootNode, "H2", &cube->h2);
			changed |= util::DragFloat(this, activeMapView->map, activeMapView->map->rootNode, "H3", &cube->h3);
			changed |= util::DragFloat(this, activeMapView->map, activeMapView->map->rootNode, "H4", &cube->h4);

			static const char* tileNames[] = { "Tile Up", "Tile Front", "Tile Side" };
			for (int t = 0; t < 3; t++)
			{
				ImGui::PushID(t);
				changed |= util::DragInt(this, activeMapView->map, activeMapView->map->rootNode, tileNames[t], &cube->tileIds[t], 1.0f, 0, (int)gnd->tiles.size() - 1);
				if (cube->tileIds[t] >= 0)
				{
					if (ImGui::TreeNodeEx(("Edit " + std::string(tileNames[t])).c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
					{
						auto tile = gnd->tiles[cube->tileIds[t]];
						int lightmapId = tile->lightmapIndex;
						if (changed |= util::DragInt(this, activeMapView->map, activeMapView->map->rootNode, "LightmapID", &lightmapId, 1.0, 0, (int)gnd->lightmaps.size() - 1))
							tile->lightmapIndex = lightmapId;

						int texIndex = tile->textureIndex;
						if (changed |= util::DragInt(this, activeMapView->map, activeMapView->map->rootNode, "TextureID", &texIndex, 1.0f, 0, (int)gnd->textures.size() - 1))
							tile->textureIndex = texIndex;
						glm::vec4 color = glm::vec4(tile->color) / 255.0f;
						if (changed |= util::ColorEdit4(this, activeMapView->map, activeMapView->map->rootNode, "Color", &color))
						{
							tile->color = glm::ivec4(color * 255.0f);
							for (auto tt : activeMapView->map->tileSelection)
								if (gnd->cubes[tt.x][tt.y]->tileIds[t] != -1)
									gnd->tiles[gnd->cubes[tt.x][tt.y]->tileIds[t]]->color = tile->color;
						}
						changed |= util::DragFloat2(this, activeMapView->map, activeMapView->map->rootNode, "UV1", &tile->v1, 0.01f, 0.0f, 1.0f);
						changed |= util::DragFloat2(this, activeMapView->map, activeMapView->map->rootNode, "UV2", &tile->v2, 0.01f, 0.0f, 1.0f);
						changed |= util::DragFloat2(this, activeMapView->map, activeMapView->map->rootNode, "UV3", &tile->v3, 0.01f, 0.0f, 1.0f);
						changed |= util::DragFloat2(this, activeMapView->map, activeMapView->map->rootNode, "UV4", &tile->v4, 0.01f, 0.0f, 1.0f);

						{ //UV editor

							ImGui::DragFloat("Snap size", &activeMapView->gridSizeTranslate, 0.125);

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
							if(gndRenderer->textures.size() > tile->textureIndex && gndRenderer->textures[tile->textureIndex]->loaded)
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
									bool snap = activeMapView->snapToGrid;
									if (ImGui::GetIO().KeyShift)
										snap = !snap;

									if (snap)
									{
										tile->texCoords[i] = glm::round(tile->texCoords[i] / activeMapView->gridSizeTranslate) * activeMapView->gridSizeTranslate;
									}
									gndRenderer->setChunkDirty(activeMapView->map->tileSelection[0].x, activeMapView->map->tileSelection[0].y);
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
							glm::vec2 lm1(0,0);
							glm::vec2 lm2(1,1);
							ImGuiWindow* window = ImGui::GetCurrentWindow();
							ImGui::Text("Shadow");
							const ImGuiStyle& style = ImGui::GetStyle();
							const float avail = ImGui::GetContentRegionAvailWidth();
							const float dim = ImMin(avail, 300.0f);
							ImVec2 Canvas(dim, dim);
							ImRect bb(window->DC.CursorPos, window->DC.CursorPos + Canvas);
							ImGui::ItemSize(bb);
							ImGui::ItemAdd(bb, NULL);
							ImGui::RenderFrame(ImVec2(bb.Min.x - 2, bb.Min.y - 2), ImVec2(bb.Max.x + 2, bb.Max.y + 2), ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);


							static gl::Texture* shadow = new gl::Texture(gnd->lightmapWidth, gnd->lightmapHeight);
							if (shadow->width != gnd->lightmapWidth || shadow->height != gnd->lightmapHeight)
								shadow->resize(gnd->lightmapWidth, gnd->lightmapHeight);
							window->DrawList->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd)
							{
								glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureId);
								GndRenderer* gndRenderer = (GndRenderer*)cmd->UserCallbackData;
								glDisable(GL_BLEND);
								shadow->bind();
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							}, gndRenderer);

							auto lm = gnd->lightmaps[lightmapId];
							unsigned char* data = new unsigned char[gnd->lightmapWidth * gnd->lightmapHeight * 4];
							for (int x = 0; x < gnd->lightmapWidth; x++)
							{
								for (int y = 0; y < gnd->lightmapHeight; y++)
								{
									data[4 * (x + gnd->lightmapWidth * y) + 0] = lm->data[x + gnd->lightmapWidth * y];
									data[4 * (x + gnd->lightmapWidth * y) + 1] = lm->data[x + gnd->lightmapWidth * y];
									data[4 * (x + gnd->lightmapWidth * y) + 2] = lm->data[x + gnd->lightmapWidth * y];
									data[4 * (x + gnd->lightmapWidth * y) + 3] = 255;
								}
							}
							shadow->setSubImage((char*)data, 0, 0, gnd->lightmapWidth, gnd->lightmapHeight);

							window->DrawList->AddImage((ImTextureID)(long long)shadow->id(), bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), ImVec2(lm1.x, lm2.y), ImVec2(lm2.x, lm1.y));
							window->DrawList->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd)
							{
								GndRenderer* gndRenderer = (GndRenderer*)cmd->UserCallbackData;
								glEnable(GL_BLEND);
								shadow->bind();
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
								glBindTexture(GL_TEXTURE_2D, oldTextureId);
							}, gndRenderer);


							bb = ImRect(window->DC.CursorPos, window->DC.CursorPos + Canvas);
							ImGui::ItemSize(bb);
							ImGui::ItemAdd(bb, NULL);
							ImGui::RenderFrame(ImVec2(bb.Min.x - 2, bb.Min.y - 2), ImVec2(bb.Max.x + 2, bb.Max.y + 2), ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);


							static gl::Texture* color = new gl::Texture(gnd->lightmapWidth, gnd->lightmapHeight);
							if (color->width != gnd->lightmapWidth || shadow->height != gnd->lightmapHeight)
								color->resize(gnd->lightmapWidth, gnd->lightmapHeight);
							window->DrawList->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd)
							{
								glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureId);
								GndRenderer* gndRenderer = (GndRenderer*)cmd->UserCallbackData;
								glDisable(GL_BLEND);
								color->bind();
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
							}, gndRenderer);

							for (int x = 0; x < gnd->lightmapWidth; x++)
							{
								for (int y = 0; y < gnd->lightmapHeight; y++)
								{
									data[4 * (x + gnd->lightmapWidth * y) + 0] = lm->data[gnd->lightmapOffset() + 3 * (x + gnd->lightmapWidth * y) + 0];
									data[4 * (x + gnd->lightmapWidth * y) + 1] = lm->data[gnd->lightmapOffset() + 3 * (x + gnd->lightmapWidth * y) + 1];
									data[4 * (x + gnd->lightmapWidth * y) + 2] = lm->data[gnd->lightmapOffset() + 3 * (x + gnd->lightmapWidth * y) + 2];
									data[4 * (x + gnd->lightmapWidth * y) + 3] = 255;
								}
							}
							color->setSubImage((char*)data, 0, 0, gnd->lightmapWidth, gnd->lightmapHeight);
							delete[] data;


							window->DrawList->AddImage((ImTextureID)(long long)color->id(), bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), ImVec2(lm1.x, lm2.y), ImVec2(lm2.x, lm1.y));
							window->DrawList->AddCallback([](const ImDrawList* parent_list, const ImDrawCmd* cmd)
							{
								GndRenderer* gndRenderer = (GndRenderer*)cmd->UserCallbackData;
								glEnable(GL_BLEND);
								color->bind();
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
								glBindTexture(GL_TEXTURE_2D, oldTextureId);
							}, gndRenderer);
						}

						ImGui::TreePop();
					}
				}
				ImGui::PopID();
			}

			if (changed)
				gndRenderer->setChunkDirty(activeMapView->map->tileSelection[0].x, activeMapView->map->tileSelection[0].y);
			ImGui::TreePop();
		}
	}
	ImGui::PopItemWidth();
	ImGui::End();
}