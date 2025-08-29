#include <windows.h>
#include <browedit/MapView.h>
#include <browedit/BrowEdit.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <browedit/shaders/SimpleShader.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/SelectWallAction.h>
#include <browedit/actions/TilePropertyChangeAction.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>


glm::ivec2 selectionStart;
int selectionSide;
std::vector<glm::ivec3> selectedWalls;
bool previewWall = false; //move this
bool selecting = false;
bool panning = false;

void MapView::postRenderWallMode(BrowEdit* browEdit)
{
	bool dropper = wallBottomDropper || wallTopDropper;
	if (dropper)
		browEdit->cursor = browEdit->dropperCursor;


	auto gnd = map->rootNode->getComponent<Gnd>();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	auto mouse3D = gnd->rayCast(mouseRay, viewEmptyTiles);
	glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));

	glm::vec2 tileHoveredOffset((mouse3D.x / 10) - tileHovered.x, tileHovered.y - (gnd->height - mouse3D.z / 10));
	tileHoveredOffset -= glm::vec2(0.5f);

	int side = 0;

	if (glm::abs(tileHoveredOffset.x) > glm::abs(tileHoveredOffset.y))
		if (tileHoveredOffset.x < 0)
			side = 0;
		else
			side = 1;
	else
		if (tileHoveredOffset.y < 0)
			side = 2;
		else
			side = 3;
	fbo->bind();

	if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		side = selectionSide;

	if (side == 0)
	{
		tileHovered.x--;
		side = 1;
	}
	if (side == 3)
	{
		tileHovered.y--;
		side = 2;
	}

	if (!dropper && gnd->inMap(tileHovered) && (
		(side == 1 && gnd->inMap(tileHovered + glm::ivec2(1, 0))) ||
		(side == 2 && gnd->inMap(tileHovered + glm::ivec2(0, 1)))) && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		auto cube = gnd->cubes[tileHovered.x][tileHovered.y];
		glm::vec3 v1, v2, v3, v4;
		if (side == 1)
		{
			v1 = glm::vec3(10 * tileHovered.x + 10, -cube->h2, 10 * gnd->height - 10 * tileHovered.y + 10);
			v2 = glm::vec3(10 * tileHovered.x + 10, -cube->h4, 10 * gnd->height - 10 * tileHovered.y);
			v3 = glm::vec3(10 * tileHovered.x + 10, -gnd->cubes[tileHovered.x + 1][tileHovered.y]->h1, 10 * gnd->height - 10 * tileHovered.y + 10);
			v4 = glm::vec3(10 * tileHovered.x + 10, -gnd->cubes[tileHovered.x + 1][tileHovered.y]->h3, 10 * gnd->height - 10 * tileHovered.y);
		}
		if (side == 2)
		{
			v1 = glm::vec3(10 * tileHovered.x, -cube->h3, 10 * gnd->height - 10 * tileHovered.y);
			v2 = glm::vec3(10 * tileHovered.x + 10, -cube->h4, 10 * gnd->height - 10 * tileHovered.y);
			v4 = glm::vec3(10 * tileHovered.x + 10, -gnd->cubes[tileHovered.x][tileHovered.y + 1]->h2, 10 * gnd->height - 10 * tileHovered.y);
			v3 = glm::vec3(10 * tileHovered.x, -gnd->cubes[tileHovered.x][tileHovered.y + 1]->h1, 10 * gnd->height - 10 * tileHovered.y);
		}

		glDisable(GL_DEPTH_TEST);
		glLineWidth(2);
		glDisable(GL_TEXTURE_2D);
		glColor3fv(glm::value_ptr(browEdit->config.wallEditSelectionColor));
		glBegin(GL_QUADS);
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v4.x, v4.y, v4.z);
		glVertex3f(v3.x, v3.y, v3.z);
		glEnd();
		glBegin(GL_LINES);
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v4.x, v4.y, v4.z);
		glVertex3f(v3.x, v3.y, v3.z);
		glEnd();
		glEnable(GL_DEPTH_TEST);
	}
	static std::vector<Gnd::Tile> originalTiles;
	if (hovered)
	{
		if (dropper && gnd->inMap(tileHovered))
		{
			glm::vec2 tileHoveredOffset((mouse3D.x / 10) - tileHovered.x, tileHovered.y - (gnd->height - mouse3D.z / 10));
			int index = 0;
			if (tileHoveredOffset.x < 0.5 && tileHoveredOffset.y > 0.5)
				index = 0;
			if (tileHoveredOffset.x > 0.5 && tileHoveredOffset.y > 0.5)
				index = 1;
			if (tileHoveredOffset.x < 0.5 && tileHoveredOffset.y < 0.5)
				index = 2;
			if (tileHoveredOffset.x > 0.5 && tileHoveredOffset.y < 0.5)
				index = 3;

			glm::ivec2 tt = tileHovered;


			glm::vec3 point(tt.x * 10.0f + ((index % 2) == 1 ? 10.0f : 0.0f),
				-gnd->cubes[tt.x][tt.y]->heights[index],
				(gnd->height - tt.y) * 10.0f + ((index / 2) == 0 ? 10.0f : 0.0f));

			if (ImGui::IsMouseClicked(0))
			{
				if (wallBottomDropper)
				{
					wallBottom = point.y;
					wallBottomAuto = false;
				}
				if (wallTopDropper)
				{
					wallTop = point.y;
					wallTopAuto = false;
				}
				wallBottomDropper = false;
				wallTopDropper = false;
			}

		}
		else
		{
			if (!selecting && !panning)
			{
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					bool collision = false;
					for (const auto& wall : selectedWalls)
					{
						glm::ivec2 tile = glm::ivec2(wall);
						if (gnd->inMap(tile) && (
							(side == 1 && gnd->inMap(tileHovered + glm::ivec2(1, 0))) ||
							(side == 2 && gnd->inMap(tileHovered + glm::ivec2(0, 1)))))
						{
							auto cube = gnd->cubes[tile.x][tile.y];
							glm::vec3 v1, v2, v3, v4;
							if (wall.z == 1)
							{
								v1 = glm::vec3(10 * tile.x + 10, -cube->h4, 10 * gnd->height - 10 * tile.y);
								v2 = glm::vec3(10 * tile.x + 10, -cube->h2, 10 * gnd->height - 10 * tile.y + 10);
								v3 = glm::vec3(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h3, 10 * gnd->height - 10 * tile.y);
								v4 = glm::vec3(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h1, 10 * gnd->height - 10 * tile.y + 10);
							}
							if (wall.z == 2)
							{
								v1 = glm::vec3(10 * tile.x, -cube->h3, 10 * gnd->height - 10 * tile.y);
								v2 = glm::vec3(10 * tile.x + 10, -cube->h4, 10 * gnd->height - 10 * tile.y);
								v4 = glm::vec3(10 * tile.x + 10, -gnd->cubes[tile.x][tile.y + 1]->h2, 10 * gnd->height - 10 * tile.y);
								v3 = glm::vec3(10 * tile.x, -gnd->cubes[tile.x][tile.y + 1]->h1, 10 * gnd->height - 10 * tile.y);
							}

							{
								float f;
								std::vector<glm::vec3> v{ v4, v2, v1 };
								if (mouseRay.LineIntersectPolygon(v, f))
									collision = true;
							}
							{
								float f;
								std::vector<glm::vec3> v{ v4, v1, v3 };
								if (mouseRay.LineIntersectPolygon(v, f))
									collision = true;
							}

						}
					}

					if (!collision)
					{
						selectionStart = tileHovered;
						selectionSide = side;
						selecting = true;
					}
					else
					{
						originalTiles.clear();
						for (const auto& w : selectedWalls)
						{
							auto cube = gnd->cubes[w.x][w.y];
							Gnd::Tile* tile = nullptr;
							if (w.z == 1 && cube->tileSide != -1)
								tile = gnd->tiles[cube->tileSide];
							if (w.z == 2 && cube->tileFront != -1)
								tile = gnd->tiles[cube->tileFront];
							if (!tile)
								continue;
							originalTiles.push_back(Gnd::Tile(*tile));
						}
						panning = true;
						std::cout << "Panning!" << std::endl;
					}
				}
			}
			if (selecting)
			{
				if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
				{
					glm::ivec2 tile = selectionStart;
					for (int i = 0; i < 100; i++)
					{
						if (gnd->inMap(tile) && (
							(side == 1 && gnd->inMap(tileHovered + glm::ivec2(1, 0))) ||
							(side == 2 && gnd->inMap(tileHovered + glm::ivec2(0, 1)))))
						{
							auto cube = gnd->cubes[tile.x][tile.y];
							glm::vec3 v1, v2, v3, v4;
							if (selectionSide == 1)
							{
								v1 = glm::vec3(10 * tile.x + 10, -cube->h2, 10 * gnd->height - 10 * tile.y + 10);
								v2 = glm::vec3(10 * tile.x + 10, -cube->h4, 10 * gnd->height - 10 * tile.y);
								v3 = glm::vec3(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h1, 10 * gnd->height - 10 * tile.y + 10);
								v4 = glm::vec3(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h3, 10 * gnd->height - 10 * tile.y);
							}
							if (selectionSide == 2)
							{
								v1 = glm::vec3(10 * tile.x, -cube->h3, 10 * gnd->height - 10 * tile.y);
								v2 = glm::vec3(10 * tile.x + 10, -cube->h4, 10 * gnd->height - 10 * tile.y);
								v4 = glm::vec3(10 * tile.x + 10, -gnd->cubes[tile.x][tile.y + 1]->h2, 10 * gnd->height - 10 * tile.y);
								v3 = glm::vec3(10 * tile.x, -gnd->cubes[tile.x][tile.y + 1]->h1, 10 * gnd->height - 10 * tile.y);

							}
							glEnable(GL_BLEND);
							glDisable(GL_DEPTH_TEST);
							glLineWidth(2);
							glDisable(GL_TEXTURE_2D);
							glColor3fv(glm::value_ptr(browEdit->config.wallEditSelectionColor));
							glBegin(GL_LINE_LOOP);
							glVertex3f(v1.x, v1.y, v1.z);
							glVertex3f(v2.x, v2.y, v2.z);
							glVertex3f(v4.x, v4.y, v4.z);
							glVertex3f(v3.x, v3.y, v3.z);
							glEnd();
							glBegin(GL_LINES);
							glVertex3f(v1.x, v1.y, v1.z);
							glVertex3f(v2.x, v2.y, v2.z);
							glVertex3f(v4.x, v4.y, v4.z);
							glVertex3f(v3.x, v3.y, v3.z);
							glEnd();
							glEnable(GL_DEPTH_TEST);

						}
						if (selectionSide == 1 && tile.y == tileHovered.y)
							break;
						if (selectionSide == 2 && tile.x == tileHovered.x)
							break;
						if (selectionSide == 1)
							tile += glm::ivec2(0, glm::sign(tileHovered.y - selectionStart.y));
						else
							tile += glm::ivec2(glm::sign(tileHovered.x - selectionStart.x), 0);
						if (tileHovered == selectionStart)
							break;
					}
				}
				if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				{
					auto ga = new GroupAction();
					glm::ivec2 tile = selectionStart;
					for (int i = 0; i < 100; i++)
					{
						if (gnd->inMap(tile) && std::find(selectedWalls.begin(), selectedWalls.end(), glm::ivec3(tile, selectionSide)) == selectedWalls.end())
							ga->addAction(new SelectWallAction(map, glm::ivec3(tile, selectionSide), ga->isEmpty() ? ImGui::GetIO().KeyShift : true, false));
						if (selectionSide == 1 && tile.y == tileHovered.y)
							break;
						if (selectionSide == 2 && tile.x == tileHovered.x)
							break;
						if (selectionSide == 1)
							tile += glm::ivec2(0, glm::sign(tileHovered.y - selectionStart.y));
						else
							tile += glm::ivec2(glm::sign(tileHovered.x - selectionStart.x), 0);
						if (tileHovered == selectionStart)
							break;
					}
					if (!ga->isEmpty())
						map->doAction(ga, browEdit);
					else
						delete ga;
					selecting = false;
				}
			}
			if (panning)
			{
				for (const auto& w : selectedWalls)
				{
					auto cube = gnd->cubes[w.x][w.y];
					Gnd::Tile* tile = nullptr;
					if (w.z == 1 && cube->tileSide != -1)
						tile = gnd->tiles[cube->tileSide];
					if (w.z == 2 && cube->tileFront != -1)
						tile = gnd->tiles[cube->tileFront];
					if (!tile)
						continue;

					glm::vec2 delta = glm::vec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y) * 0.01f;
					if (ImGui::GetIO().KeyShift)
						delta *= 0.01f;
					if (ImGui::GetIO().KeyCtrl)
						delta.x = 0;

					glm::vec2 v1 = tile->v1;
					glm::vec2 v2 = tile->v2;
					glm::vec2 v3 = tile->v3;
					glm::vec2 v4 = tile->v4;

					for (int axis = 0; axis < 2; axis++)
					{
						tile->v1[axis] += delta[axis];
						tile->v2[axis] += delta[axis];
						tile->v3[axis] += delta[axis];
						tile->v4[axis] += delta[axis];

						if (glm::clamp(tile->v1[axis], 0.0f, 1.0f) != tile->v1[axis] ||
							glm::clamp(tile->v2[axis], 0.0f, 1.0f) != tile->v2[axis] ||
							glm::clamp(tile->v3[axis], 0.0f, 1.0f) != tile->v3[axis] ||
							glm::clamp(tile->v4[axis], 0.0f, 1.0f) != tile->v4[axis])
						{
							tile->v1[axis] = v1[axis];
							tile->v2[axis] = v2[axis];
							tile->v3[axis] = v3[axis];
							tile->v4[axis] = v4[axis];
						}
					}

					gndRenderer->setChunkDirty(w.x, w.y);
				}
				if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				{
					panning = false;
					auto ga = new GroupAction();
					int i = 0;
					for (const auto& w : selectedWalls)
					{
						auto cube = gnd->cubes[w.x][w.y];
						Gnd::Tile* tile = nullptr;
						if (w.z == 1 && cube->tileSide != -1)
							tile = gnd->tiles[cube->tileSide];
						if (w.z == 2 && cube->tileFront != -1)
							tile = gnd->tiles[cube->tileFront];
						if (!tile)
							continue;
						for (int ii = 0; ii < 4; ii++)
							ga->addAction(new TileChangeAction<glm::vec2>(w, tile, &tile->texCoords[ii], originalTiles[i].texCoords[ii], "Change UV"));
						i++;
					}
					map->doAction(ga, browEdit);
				}
			}
		}
	}
	if (selectedWalls.size() > 0 && browEdit->activeMapView)
	{
		std::vector<VertexP3T2N3> verts;
		std::vector<VertexP3T2N3> topverts;
		std::vector<VertexP3T2N3> botverts;

		WallCalculation calculation;
		calculation.prepare(browEdit);

		for (const auto& wall : selectedWalls)
		{
			glm::ivec2 tile = glm::ivec2(wall);

			if (gnd->inMap(tile) && (
				(side == 1 && gnd->inMap(tile + glm::ivec2(1, 0))) ||
				(side == 2 && gnd->inMap(tile + glm::ivec2(0, 1)))))
			{
				auto cube = gnd->cubes[tile.x][tile.y];
				glm::vec3 v1, v2, v3, v4, normal;
				if (wall.z == 1)
				{
					v1 = glm::vec3(10 * tile.x + 10, -cube->h4, 10 * gnd->height - 10 * tile.y);
					v2 = glm::vec3(10 * tile.x + 10, -cube->h2, 10 * gnd->height - 10 * tile.y + 10);
					v3 = glm::vec3(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h3, 10 * gnd->height - 10 * tile.y);
					v4 = glm::vec3(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h1, 10 * gnd->height - 10 * tile.y + 10);
					normal = glm::vec3(1, 0, 0);
				}
				if (wall.z == 2)
				{
					v1 = glm::vec3(10 * tile.x, -cube->h3, 10 * gnd->height - 10 * tile.y);
					v2 = glm::vec3(10 * tile.x + 10, -cube->h4, 10 * gnd->height - 10 * tile.y);
					v4 = glm::vec3(10 * tile.x + 10, -gnd->cubes[tile.x][tile.y + 1]->h2, 10 * gnd->height - 10 * tile.y);
					v3 = glm::vec3(10 * tile.x, -gnd->cubes[tile.x][tile.y + 1]->h1, 10 * gnd->height - 10 * tile.y);
					normal = glm::vec3(0, 0, 1);
				}
				if (previewWall)
				{
					calculation.calcUV(wall, gnd);
					verts.push_back(VertexP3T2N3(v1, calculation.g_uv1, normal));
					verts.push_back(VertexP3T2N3(v2, calculation.g_uv2, normal));
					verts.push_back(VertexP3T2N3(v4, calculation.g_uv4, normal));
					verts.push_back(VertexP3T2N3(v3, calculation.g_uv3, normal));
				}
				else
				{
					verts.push_back(VertexP3T2N3(v1, glm::vec2(0, 0), normal));
					verts.push_back(VertexP3T2N3(v2, glm::vec2(0, 0), normal));
					verts.push_back(VertexP3T2N3(v2, glm::vec2(0, 0), normal));
					verts.push_back(VertexP3T2N3(v4, glm::vec2(0, 0), normal));
					verts.push_back(VertexP3T2N3(v4, glm::vec2(0, 0), normal));
					verts.push_back(VertexP3T2N3(v3, glm::vec2(0, 0), normal));
					verts.push_back(VertexP3T2N3(v3, glm::vec2(0, 0), normal));
					verts.push_back(VertexP3T2N3(v1, glm::vec2(0, 0), normal));
				}

				if (wall.z == 2)
				{
					if (!wallTopAuto)
					{
						topverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, wallTop, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
						topverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallTop, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
					}
					if (!wallBottomAuto)
					{
						botverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallBottom, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
						botverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, wallBottom, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
					}
				}
				else
				{
					if (!wallTopAuto)
					{
						topverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallTop, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
						topverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallTop, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0, 0), normal));
					}
					if (!wallBottomAuto)
					{
						botverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallBottom, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
						botverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallBottom, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0, 0), normal));
					}
				}
			}
		}

		if (verts.size() > 0)
		{
			simpleShader->use();
			simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
			simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
			simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
			glLineWidth(2);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glEnable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glDisableVertexAttribArray(3);
			glDisableVertexAttribArray(4); //TODO: vao
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
			glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);
			if (previewWall && browEdit->activeMapView->textureSelected >= 0 && browEdit->activeMapView->textureSelected < gndRenderer->textures.size())
			{
				simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
				simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(browEdit->config.wallEditHighlightColor, 1));

				gndRenderer->textures[browEdit->activeMapView->textureSelected]->bind();
				glDrawArrays(GL_QUADS, 0, (int)verts.size());
			}
			else
			{
				simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
				simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(browEdit->config.wallEditHighlightColor, 1));
				glDrawArrays(GL_LINES, 0, (int)verts.size());
			}


			glEnable(GL_DEPTH_TEST);
		}

		if (botverts.size() > 0)
		{
			simpleShader->use();
			simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
			simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
			simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
			glLineWidth(2);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glEnable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glDisableVertexAttribArray(3);
			glDisableVertexAttribArray(4); //TODO: vao
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), botverts[0].data + 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), botverts[0].data + 3);
			glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), botverts[0].data + 5);
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 0, 1));
			glDrawArrays(GL_LINES, 0, (int)botverts.size());
			glEnable(GL_DEPTH_TEST);
		}
		if (topverts.size() > 0)
		{
			simpleShader->use();
			simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
			simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
			simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
			glLineWidth(2);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glEnable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glDisableVertexAttribArray(3);
			glDisableVertexAttribArray(4); //TODO: vao
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), topverts[0].data + 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), topverts[0].data + 3);
			glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), topverts[0].data + 5);
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0, 1, 1, 1));
			glDrawArrays(GL_LINES, 0, (int)topverts.size());
			glEnable(GL_DEPTH_TEST);
		}

	}



	fbo->unbind();
}