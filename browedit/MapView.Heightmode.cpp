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
#include <browedit/components/Rsw.h>
#include <browedit/math/Polygon.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/TileSelectAction.h>
#include <browedit/actions/CubeHeightChangeAction.h>
#include <browedit/actions/CubeTileChangeAction.h>
#include <browedit/actions/NewObjectAction.h>
#include <browedit/actions/SelectAction.h>
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
	ImGui::Text("Cursor: %d,%d", tileHovered.x, tileHovered.y);
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


		if (ImGui::IsMouseReleased(0) && hovered)
		{ //paste
			std::vector<glm::ivec2> cubeSelected;
			for (auto c : browEdit->newCubes)
				if(gnd->inMap(c->pos + tileHovered))
					cubeSelected.push_back(c->pos + tileHovered);
			auto action1 = new CubeHeightChangeAction<Gnd, Gnd::Cube>(gnd, cubeSelected);
			auto action2 = new CubeTileChangeAction(gnd, cubeSelected);
			for (auto cube : browEdit->newCubes)
			{
				if (!gnd->inMap(tileHovered + cube->pos))
					continue;
				if ((browEdit->pasteOptions & PasteOptions::Height) != 0)
				{ // paste the height, easy
					for (int i = 0; i < 4; i++)
						gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->heights[i] = cube->heights[i];
					for (int i = 0; i < 4; i++)
						gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->normals[i] = cube->normals[i];
					gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->normal = cube->normal;
				}
				for (int i = 0; i < 3; i++)
				{ // paste tiles
					if (i > 0 && (browEdit->pasteOptions & PasteOptions::Walls) == 0)
						continue;
					int tileId = cube->tileIds[i];
					if (tileId == -1)
					{ // clear the new tile
						gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->tileIds[i] = tileId;
						continue;
					}

					Gnd::Tile* oldTile = nullptr;
					if(gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->tileIds[i] > -1)
						oldTile = gnd->tiles[gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->tileIds[i]];

					gnd->cubes[tileHovered.x + cube->pos.x][tileHovered.y + cube->pos.y]->tileIds[i] = (int)gnd->tiles.size();
					Gnd::Tile* newTile = nullptr;
					if (oldTile != nullptr)
						newTile = new Gnd::Tile(*oldTile); //base it on the old tile
					else
						newTile = new Gnd::Tile();

					if ((browEdit->pasteOptions & PasteOptions::Textures) != 0)
					{ //find the textureId and set texcoords
						int texId = -1;
						for (auto t = 0; t < gnd->textures.size(); t++)
							if (*gnd->textures[t] == cube->texture[i])
								texId = t;
						if (texId == -1)
						{
							texId = (int)gnd->textures.size();
							gnd->textures.push_back(new Gnd::Texture(cube->texture[i]));
						}
						newTile->textureIndex = texId;
						for (int ii = 0; ii < 4; ii++)
							newTile->texCoords[ii] = cube->tile[i].texCoords[ii];
					}
					if ((browEdit->pasteOptions & PasteOptions::Colors) != 0)
						newTile->color = cube->tile[i].color;
					if ((browEdit->pasteOptions & PasteOptions::Lightmaps) != 0)
					{ //TODO: check if cube->lightmap can be null?
						auto lm = new Gnd::Lightmap(cube->lightmap[i]);
						newTile->lightmapIndex = (int)gnd->lightmaps.size();
						gnd->lightmaps.push_back(lm);	//TODO: check if the lightmap already exists on the map.. not super important if lightmap deduplication is done at saving
					}
					gnd->tiles.push_back(newTile);
				}
			}
			action1->setNewHeights(gnd, cubeSelected);
			action2->setNewTiles(gnd, cubeSelected);

			auto ga = new GroupAction();
			ga->addAction(action1);
			ga->addAction(action2);
			ga->addAction(new TileSelectAction(map, cubeSelected));

			if ((browEdit->pasteOptions & PasteOptions::Objects) != 0)
			{
				bool first = false;
				glm::ivec2 center(browEdit->pasteData["center"]);
				glm::ivec2 offset = tileHovered - center;
				glm::vec3 centerObjects(center.x * 10 - 5 * gnd->width, 0, center.y * 10 - 5 * gnd->height);

				for (auto newNode : browEdit->pasteData["objects"])
				{
					Node* n = new Node(newNode["name"].get<std::string>());
					n->addComponentsFromJson(newNode["components"]);
					glm::vec3 relative_position;
					from_json(newNode["relative_position"], relative_position);
					std::string path = n->name;
					if (path.find("\\") != std::string::npos)
						path = path.substr(0, path.rfind("\\"));
					else
						path = "";
					n->setParent(map->findAndBuildNode(path));
					n->makeNameUnique(map->rootNode);
					n->getComponent<RswObject>()->position = relative_position + glm::vec3(10 * offset.x, 0, 10 * offset.y) + centerObjects;
					ga->addAction(new NewObjectAction(n));
					auto sa = new SelectAction(map, n, first, false);
					sa->perform(map, browEdit); // to make sure everything is selected
					ga->addAction(sa);
					first = true;
				}
			}

			if ((browEdit->pasteOptions & PasteOptions::GAT) != 0)
			{
				auto gat = map->rootNode->getComponent<Gat>();
				std::vector<glm::ivec2> cubeSelected;
				for (auto c : browEdit->pasteData["gats"])
				{
					glm::ivec2 pos(c["pos"]);
					if (gat->inMap(pos + 2*tileHovered))
						cubeSelected.push_back(pos + 2*tileHovered);
				}
				ga->addAction(new GatTileSelectAction(map, cubeSelected));
				auto action1 = new CubeHeightChangeAction<Gat, Gat::Cube>(gat, cubeSelected);

				for (auto c : browEdit->pasteData["gats"])
				{
					glm::ivec2 pos = glm::ivec2(c["pos"]) + 2 * tileHovered;
					if (gat->inMap(pos))
						from_json(c, *gat->cubes[pos.x][pos.y]);
				}

				action1->setNewHeights(gat, cubeSelected);
				ga->addAction(action1);
			}
			map->doAction(ga, browEdit);


			for (auto c : browEdit->newCubes)
				delete c;
			browEdit->newCubes.clear();
		}


	}

	if (browEdit->heightDoodle && hovered)
	{
		static std::map<Gnd::Cube*, float[4]> originalHeights;
		static std::vector<glm::ivec2> tilesProcessed;

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

		
		float minMax = ImGui::GetIO().KeyShift ? std::numeric_limits<float>::max() : -std::numeric_limits<float>::max();
		if (ImGui::GetIO().KeyCtrl)
		{
			minMax = ImGui::GetIO().KeyShift ? -std::numeric_limits<float>::max() : std::numeric_limits<float>::max();
			for (int x = 0; x <= browEdit->windowData.heightEdit.doodleSize; x++)
			{
				for (int y = 0; y <= browEdit->windowData.heightEdit.doodleSize; y++)
				{
					glm::ivec2 t = tileHovered + glm::ivec2(x, y) - glm::ivec2(browEdit->windowData.heightEdit.doodleSize / 2);
					for (int ii = 0; ii < 4; ii++)
					{
						glm::ivec2 tt(t.x + gnd->connectInfo[index][ii].x, t.y + gnd->connectInfo[index][ii].y);
						if (!gnd->inMap(tt))
							continue;
						int ti = gnd->connectInfo[index][ii].z;
						if (ImGui::GetIO().KeyShift)
							minMax = glm::max(minMax, gnd->cubes[tt.x][tt.y]->heights[ti]);
						else
							minMax = glm::min(minMax, gnd->cubes[tt.x][tt.y]->heights[ti]);
					}
				}
			}
		}

		ImGui::Begin("Statusbar");
		ImGui::Text("Change Height, hold shift to lower, hold Ctrl to snap");
		ImGui::End();

		for (int x = 0; x <= browEdit->windowData.heightEdit.doodleSize; x++)
		{
			for (int y = 0; y <= browEdit->windowData.heightEdit.doodleSize; y++)
			{
				glm::ivec2 t = tileHovered + glm::ivec2(x, y) - glm::ivec2(browEdit->windowData.heightEdit.doodleSize/2);
				for (int ii = 0; ii < 4; ii++)
				{
					glm::ivec2 tt(t.x + gnd->connectInfo[index][ii].x, t.y + gnd->connectInfo[index][ii].y);
					if (!gnd->inMap(tt))
						continue;
					int ti = gnd->connectInfo[index][ii].z;
					float& snapHeight = gnd->cubes[tt.x][tt.y]->heights[ti];
					if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
					{
						if (originalHeights.find(gnd->cubes[tt.x][tt.y]) == originalHeights.end()) {
							for (int i = 0; i < 4; i++)
								originalHeights[gnd->cubes[tt.x][tt.y]][i] = gnd->cubes[tt.x][tt.y]->heights[i];
							tilesProcessed.push_back(tt);
						}

						if (ImGui::GetIO().KeyShift)
							snapHeight = glm::min(minMax, snapHeight + browEdit->windowData.heightEdit.doodleSpeed * (glm::mix(1.0f, glm::max(0.0f, 1.0f - glm::length(glm::vec2(x - browEdit->windowData.heightEdit.doodleSize / 2.0f, y - browEdit->windowData.heightEdit.doodleSize / 2.0f))/ browEdit->windowData.heightEdit.doodleSize), browEdit->windowData.heightEdit.doodleHardness)));
						else
							snapHeight = glm::max(minMax, snapHeight - browEdit->windowData.heightEdit.doodleSpeed * (glm::mix(1.0f, glm::max(0.0f, 1.0f - glm::length(glm::vec2(x - browEdit->windowData.heightEdit.doodleSize / 2.0f, y - browEdit->windowData.heightEdit.doodleSize / 2.0f)) / browEdit->windowData.heightEdit.doodleSize), browEdit->windowData.heightEdit.doodleHardness)));
						gnd->cubes[tt.x][tt.y]->calcNormal();
						gndRenderer->setChunkDirty(tt.x, tt.y);
					}
					glUseProgram(0);
					glPointSize(10.0);
					glBegin(GL_POINTS);
					glVertex3f(tt.x * 10.0f + ((ti % 2) == 1 ? 10.0f : 0.0f),
						-snapHeight + 1,
						(gnd->height - tt.y) * 10.0f + ((ti / 2) == 0 ? 10.0f : 0.0f));
					glEnd();
				}
			}
		}

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			if (originalHeights.size() > 0)
			{
				std::map<Gnd::Cube*, float[4]> newHeights;
				for(auto kv : originalHeights)
					for (int i = 0; i < 4; i++)
						newHeights[kv.first][i] = kv.first->heights[i];

				map->doAction(new CubeHeightChangeAction<Gnd, Gnd::Cube>(originalHeights, newHeights, tilesProcessed), browEdit);
			}
			originalHeights.clear();
			tilesProcessed.clear();
		}

	}
	else
	{
		//draw selection
		if (map->tileSelection.size() > 0 && canSelect)
		{
			std::vector<VertexP3T2N3> verts;
			float dist = 0.002f * cameraDistance;
			for (auto& tile : map->tileSelection)
			{
				auto cube = gnd->cubes[tile.x][tile.y];
				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x,		-cube->h3 + dist, 10 * gnd->height - 10 * tile.y), glm::vec2(0), cube->normals[2]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10,-cube->h2 + dist, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0), cube->normals[1]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10,-cube->h4 + dist, 10 * gnd->height - 10 * tile.y), glm::vec2(0), cube->normals[3]));

				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x,		-cube->h1 + dist, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0), cube->normals[0]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x,		-cube->h3 + dist, 10 * gnd->height - 10 * tile.y), glm::vec2(0), cube->normals[2]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10,-cube->h2 + dist, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0), cube->normals[1]));
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
			glm::vec3 posGadget[9];
			posGadget[0] = pos[0] = gnd->getPos(minValues.x, minValues.y, 0);
			posGadget[1] = pos[1] = gnd->getPos(maxValues.x, minValues.y, 1);
			posGadget[2] = pos[2] = gnd->getPos(minValues.x, maxValues.y, 2);
			posGadget[3] = pos[3] = gnd->getPos(maxValues.x, maxValues.y, 3);
			glm::vec3 center = (pos[0] + pos[1] + pos[2] + pos[3]) / 4.0f;
			
			float minY = center.y;

			int cx = (maxValues.x - minValues.x) / 2 + minValues.x;
			int cy = (maxValues.y - minValues.y) / 2 + minValues.y;
			auto cube = gnd->cubes[cx][cy];

			center.y = (cube->h3 - cube->h2) * 0.5f + cube->h2;
			
			bool xSelectionEven = (maxValues.x - minValues.x + 1) % 2 == 0;
			bool ySelectionEven = (maxValues.y - minValues.y + 1) % 2 == 0;

			if (map->tileSelection.size() > 1) {
				if (xSelectionEven && ySelectionEven)
					center.y = glm::min(glm::min(glm::min(cube->h4, gnd->cubes[cx + 1][cy]->h3), gnd->cubes[cx][cy + 1]->h2), gnd->cubes[cx + 1][cy + 1]->h1);
				else if (xSelectionEven && cx + 1 < gnd->width)
					center.y = glm::min((cube->h4 - cube->h2) * 0.5f + cube->h2, (gnd->cubes[cx + 1][cy]->h3 - gnd->cubes[cx + 1][cy]->h1) * 0.5f + gnd->cubes[cx + 1][cy]->h1);
				else if (ySelectionEven && cy + 1 < gnd->height)
					center.y = glm::min((cube->h3 - cube->h4) * 0.5f + cube->h4, (gnd->cubes[cx][cy + 1]->h1 - gnd->cubes[cx][cy + 1]->h2) * 0.5f + gnd->cubes[cx][cy + 1]->h2);
			}

			center.y = -center.y;
			
			posGadget[4] = pos[4] = center;

			glm::vec3 edge = (pos[0] + pos[1]) / 2.0f;
			edge.y = -(xSelectionEven ? glm::min(gnd->cubes[cx][minValues.y]->h2, gnd->cubes[cx + 1][minValues.y]->h1) : (gnd->cubes[cx][minValues.y]->h2 - gnd->cubes[cx][minValues.y]->h1) * 0.5f + gnd->cubes[cx][minValues.y]->h1);
			posGadget[5] = pos[5] = edge;

			edge = (pos[2] + pos[3]) / 2.0f;
			edge.y = -(xSelectionEven ? glm::min(gnd->cubes[cx][maxValues.y]->h4, gnd->cubes[cx + 1][maxValues.y]->h3) : (gnd->cubes[cx][maxValues.y]->h4 - gnd->cubes[cx][maxValues.y]->h3) * 0.5f + gnd->cubes[cx][maxValues.y]->h3);
			posGadget[6] = pos[6] = edge;

			edge = (pos[0] + pos[2]) / 2.0f;
			edge.y = -(ySelectionEven ? glm::min(gnd->cubes[minValues.x][cy]->h3, gnd->cubes[minValues.x][cy + 1]->h1) : (gnd->cubes[minValues.x][cy]->h3 - gnd->cubes[minValues.x][cy]->h1) * 0.5f + gnd->cubes[minValues.x][cy]->h1);
			posGadget[7] = pos[7] = edge;

			edge = (pos[1] + pos[3]) / 2.0f;
			edge.y = -(ySelectionEven ? glm::min(gnd->cubes[maxValues.x][cy]->h4, gnd->cubes[maxValues.x][cy + 1]->h2) : (gnd->cubes[maxValues.x][cy]->h4 - gnd->cubes[maxValues.x][cy]->h2) * 0.5f + gnd->cubes[maxValues.x][cy]->h2);
			posGadget[8] = pos[8] = edge;

			ImGui::Begin("Height Edit");
			if (ImGui::CollapsingHeader("Selection Details", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat("Corner 1", &pos[0].y);
				ImGui::DragFloat("Corner 2", &pos[1].y);
				ImGui::DragFloat("Corner 3", &pos[2].y);
				ImGui::DragFloat("Corner 4", &pos[3].y);
			}
			ImGui::End();

			glm::vec3 cameraPos = glm::inverse(nodeRenderContext.viewMatrix) * glm::vec4(0, 0, 0, 1);
			int order[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
			std::sort(std::begin(order), std::end(order), [&](int a, int b)
			{
				return glm::distance(cameraPos, pos[a]) < glm::distance(cameraPos, pos[b]);
			});

			static int dragIndex = -1;
			for (int ii = 0; ii < 9; ii++)
			{
				if (gadgetScale == 0)
					continue;
				int i = order[ii];

				if (!browEdit->windowData.heightEdit.showCornerArrows && i < 4)
					continue;
				if (!browEdit->windowData.heightEdit.showCenterArrow && i == 4)
					continue;
				if (!browEdit->windowData.heightEdit.showEdgeArrows && i > 4)
					continue;
				glm::mat4 mat = glm::translate(glm::mat4(1.0f), posGadget[i]);
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
					map->doAction(new CubeHeightChangeAction<Gnd, Gnd::Cube>(originalValues, newValues, map->tileSelection), browEdit);
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
						if (gndRenderer) {
							gndRenderer->setChunkDirty(t.x, t.y);

							// For walls on the chunk's edge
							if (gnd->inMap(glm::ivec2(t.x, t.y - 1)))
								gndRenderer->setChunkDirty(t.x, t.y - 1);

							if (gnd->inMap(glm::ivec2(t.x - 1, t.y)))
								gndRenderer->setChunkDirty(t.x - 1, t.y);
						}
						for (int ii = 0; ii < 4; ii++)
						{
							// 0 1
							// 2 3
							glm::vec3 p = gnd->getPos(t.x, t.y, ii);
							if (!browEdit->windowData.heightEdit.splitTriangleFlip)	// /
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
							if (browEdit->windowData.heightEdit.edgeMode == 1 || browEdit->windowData.heightEdit.edgeMode == 2) //smooth raise area around these tiles
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
						if (browEdit->windowData.heightEdit.edgeMode == 1) //raise ground
						{
							cube->heights[0] = originalValues[cube][0] - getPointHeightDiff(a.x - 1, a.y - 1, 3, a.x - 1, a.y, 1, a.x, a.y - 1, 2);
							cube->heights[1] = originalValues[cube][1] - getPointHeightDiff(a.x + 1, a.y - 1, 2, a.x + 1, a.y, 0, a.x, a.y - 1, 3);
							cube->heights[2] = originalValues[cube][2] - getPointHeightDiff(a.x - 1, a.y + 1, 1, a.x - 1, a.y, 3, a.x, a.y + 1, 0);
							cube->heights[3] = originalValues[cube][3] - getPointHeightDiff(a.x + 1, a.y + 1, 0, a.x + 1, a.y, 2, a.x, a.y + 1, 1);
						}
						else if (browEdit->windowData.heightEdit.edgeMode == 2) //snap ground
						{
							cube->heights[0] = getPointHeight(a.x - 1, a.y - 1, 3, a.x - 1, a.y, 1, a.x, a.y - 1, 2, cube->heights[0]);
							cube->heights[1] = getPointHeight(a.x + 1, a.y - 1, 2, a.x + 1, a.y, 0, a.x, a.y - 1, 3, cube->heights[1]);
							cube->heights[2] = getPointHeight(a.x - 1, a.y + 1, 1, a.x - 1, a.y, 3, a.x, a.y + 1, 0, cube->heights[2]);
							cube->heights[3] = getPointHeight(a.x + 1, a.y + 1, 0, a.x + 1, a.y, 2, a.x, a.y + 1, 1, cube->heights[3]);
						}
						cube->calcNormal();
					}

					if (browEdit->windowData.heightEdit.edgeMode == 3 && gnd->tiles.size() > 0) // add walls
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
								gnd->cubes[t.x - 1][t.y]->tileSide = 0;

							if (t.x < gnd->width-1 && !isTileSelected(t.x + 1, t.y) && (
								gnd->cubes[t.x][t.y]->h2 != gnd->cubes[t.x + 1][t.y]->h1 ||
								gnd->cubes[t.x][t.y]->h4 != gnd->cubes[t.x + 1][t.y]->h3) &&
								gnd->cubes[t.x][t.y]->tileSide == -1)
								gnd->cubes[t.x][t.y]->tileSide = 0;

							if (t.y > 0 && !isTileSelected(t.x, t.y - 1) && (
								gnd->cubes[t.x][t.y]->h1 != gnd->cubes[t.x][t.y - 1]->h3 ||
								gnd->cubes[t.x][t.y]->h2 != gnd->cubes[t.x][t.y - 1]->h4) &&
								gnd->cubes[t.x][t.y - 1]->tileFront == -1)
								gnd->cubes[t.x][t.y - 1]->tileFront = 0;

							if (t.y < gnd->height-1 && !isTileSelected(t.x, t.y + 1) && (
								gnd->cubes[t.x][t.y]->h3 != gnd->cubes[t.x][t.y + 1]->h1 ||
								gnd->cubes[t.x][t.y]->h4 != gnd->cubes[t.x][t.y + 1]->h2) &&
								gnd->cubes[t.x][t.y]->tileFront == -1)
								gnd->cubes[t.x][t.y]->tileFront = 0;

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

				std::map<int, glm::ivec2> newSelection;
				if (ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl) {
					for (auto& tile : map->tileSelection) {
						newSelection[tile.x + gnd->width * tile.y] = tile;
					}
				}
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
									if (newSelection.contains(x + gnd->width * y))
										newSelection.erase(x + gnd->width * y);
								}
								else if (!newSelection.contains(x + gnd->width * y))
									newSelection[x + gnd->width * y] = glm::ivec2(x, y);
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
									if (newSelection.contains(x + gnd->width * y))
										newSelection.erase(x + gnd->width * y);
								}
								else if (!newSelection.contains(x + gnd->width * y))
									newSelection[x + gnd->width * y] = glm::ivec2(x, y);
						}
					}
					for (auto t : selectLasso)
						if (ImGui::GetIO().KeyCtrl)
						{
							if (newSelection.contains(t.x + gnd->width * t.y))
								newSelection.erase(t.x + gnd->width * t.y);
						}
						else if (!newSelection.contains(t.x + gnd->width * t.y))
							newSelection[t.x + gnd->width * t.y] = glm::ivec2(t.x, t.y);
					selectLasso.clear();
				}
				else if (browEdit->selectTool == BrowEdit::SelectTool::WandTex)
				{
					glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));

					std::map<int, glm::ivec2> tilesToFill;
					std::queue<glm::ivec2> queue;
					std::map<int, bool> done;

					glm::ivec2 offsets[4] = { glm::ivec2(-1,0), glm::ivec2(1,0), glm::ivec2(0,-1), glm::ivec2(0,1) };
					queue.push(tileHovered);
					int c = 0;
					while (!queue.empty())
					{
						auto p = queue.front();
						queue.pop();
						if (!gnd->inMap(p) || gnd->cubes[p.x][p.y]->tileUp == -1)
							continue;
						for (const auto& o : offsets)
						{
							auto pp = p + o;
							if (!gnd->inMap(pp))
								continue;
							auto c = gnd->cubes[pp.x][pp.y];
							if (c->tileUp == -1)
								continue;
							auto t = gnd->tiles[c->tileUp];
							if (t->textureIndex != gnd->tiles[gnd->cubes[p.x][p.y]->tileUp]->textureIndex)
								continue;
							if (!done.contains(pp.x + gnd->width * pp.y))
							{
								done[pp.x + gnd->width * pp.y] = true;
								queue.push(pp);
							}
						}
						if (gnd->inMap(p) && !tilesToFill.contains(p.x + gnd->width * p.y))
							tilesToFill[p.x + gnd->width * p.y] = p;
						c++;
					}
					for (const auto& entry : tilesToFill)
					{
						auto t = entry.second;
						if (ImGui::GetIO().KeyCtrl)
						{
							if (newSelection.contains(t.x + gnd->width * t.y))
								newSelection.erase(t.x + gnd->width * t.y);
						}
						else if (!newSelection.contains(t.x + gnd->width * t.y))
							newSelection[t.x + gnd->width * t.y] = t;
					}
				}
				else if (browEdit->selectTool == BrowEdit::SelectTool::WandHeight)
				{
					glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));

					std::map<int, glm::ivec2> tilesToFill;
					std::queue<glm::ivec2> queue;
					std::map<int, bool> done;

					glm::ivec2 offsets[4] = { glm::ivec2(-1,0), glm::ivec2(1,0), glm::ivec2(0,-1), glm::ivec2(0,1) };
					queue.push(tileHovered);
					int c = 0;
					while (!queue.empty())
					{
						auto p = queue.front();
						queue.pop();
						if (!gnd->inMap(p))
							continue;
						for (const auto& o : offsets)
						{
							auto pp = p + o;
							if (!gnd->inMap(pp))
								continue;
							auto c = gnd->cubes[pp.x][pp.y];

							if (!c->sameHeight(*gnd->cubes[p.x][p.y]))
								continue;
							if (!done.contains(pp.x + gnd->width * pp.y))
							{
								done[pp.x + gnd->width * pp.y] = true;
								queue.push(pp);
							}
						}
						if (gnd->inMap(p) && !tilesToFill.contains(p.x + gnd->width * p.y))
							tilesToFill[p.x + gnd->width * p.y] = p;
						c++;
					}
					for (const auto& entry : tilesToFill)
					{
						auto t = entry.second;
						if (ImGui::GetIO().KeyCtrl)
						{
							if (newSelection.contains(t.x + gnd->width * t.y))
								newSelection.erase(t.x + gnd->width * t.y);
						}
						else if (!newSelection.contains(t.x + gnd->width * t.y))
							newSelection[t.x + gnd->width * t.y] = t;
					}
				}
				else if (browEdit->selectTool == BrowEdit::SelectTool::AllTex)
				{
					glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));
					if (gnd->inMap(tileHovered))
					{
						std::vector<glm::ivec2> tilesToFill;
						auto c = gnd->cubes[tileHovered.x][tileHovered.y];
						for (int x = 0; x < gnd->width; x++)
							for (int y = 0; y < gnd->height; y++)
								if (c->tileUp != -1 && gnd->cubes[x][y]->tileUp != -1 && gnd->tiles[c->tileUp]->textureIndex == gnd->tiles[gnd->cubes[x][y]->tileUp]->textureIndex)
									tilesToFill.push_back(glm::ivec2(x, y));

						for (const auto& t : tilesToFill)
						{
							if (ImGui::GetIO().KeyCtrl)
							{
								if (newSelection.contains(t.x + gnd->width * t.y))
									newSelection.erase(t.x + gnd->width * t.y);
							}
							else if (!newSelection.contains(t.x + gnd->width * t.y))
								newSelection[t.x + gnd->width * t.y] = t;
						}
					}
				}
				else if (browEdit->selectTool == BrowEdit::SelectTool::AllHeight)
				{
					glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));
					if (gnd->inMap(tileHovered))
					{
						std::vector<glm::ivec2> tilesToFill;
						auto c = gnd->cubes[tileHovered.x][tileHovered.y];
						for (int x = 0; x < gnd->width; x++)
							for (int y = 0; y < gnd->height; y++)
								if (c->sameHeight(*gnd->cubes[x][y]))
									tilesToFill.push_back(glm::ivec2(x, y));
						for (const auto& t : tilesToFill)
						{
							if (ImGui::GetIO().KeyCtrl)
							{
								if (newSelection.contains(t.x + gnd->width * t.y))
									newSelection.erase(t.x + gnd->width * t.y);
							}
							else if (!newSelection.contains(t.x + gnd->width * t.y))
								newSelection[t.x + gnd->width * t.y] = t;
						}
					}
				}

				std::vector<glm::ivec2> newSelection2;

				for (const auto& entry : newSelection) {
					newSelection2.push_back(entry.second);
				}

				if (map->tileSelection != newSelection2)
				{
					map->doAction(new TileSelectAction(map, newSelection2), browEdit);
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
							verts.push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * y + 10), glm::vec2(0), cube->normals[1]));
							verts.push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * y), glm::vec2(0), cube->normals[3]));

							verts.push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h1 + dist, 10 * gnd->height - 10 * y + 10), glm::vec2(0), cube->normals[0]));
							verts.push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h3 + dist, 10 * gnd->height - 10 * y), glm::vec2(0), cube->normals[2]));
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
	}

	glDepthMask(1);






	fbo->unbind();
}