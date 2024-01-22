#define IMGUI_DEFINE_MATH_OPERATORS
#include <windows.h>
#include "MapView.h"
#include <browedit/BrowEdit.h>
#include <browedit/shaders/SimpleShader.h>
#include <browedit/gl/FBO.h>
#include <browedit/Node.h>
#include <browedit/Map.h>
#include <browedit/gl/Texture.h>
#include <browedit/math/Polygon.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/TileNewAction.h>
#include <browedit/actions/TileSelectAction.h>
#include <browedit/actions/CubeTileChangeAction.h>
#include <browedit/actions/GndTextureActions.h>

#include <imgui_internal.h>
#include <queue>

void MapView::postRenderTextureMode(BrowEdit* browEdit)
{
	auto gnd = map->rootNode->getComponent<Gnd>();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();

	if (gndRenderer->textures.size() == 0)
		return;

	simpleShader->use();
	simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
	simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);


	fbo->bind();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_2D);

	if (textureGridVbo->size() == 0 || textureGridDirty)
	{
		textureGridDirty = false;
		buildTextureGrid();
	}
	if(textureGridVbo->size() > 0 && snapToGrid)
	{
		simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		glLineWidth(2);
		textureGridVbo->bind();
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4); //TODO: vao
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(0 * sizeof(float)));
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(3 * sizeof(float)));
		glDrawArrays(GL_LINES, 0, (int)textureGridVbo->size());
		textureGridVbo->unBind();
	}

	auto mouse3D = gnd->rayCast(mouseRay, viewEmptyTiles);
	glm::vec2 uvSize = textureEditUv2 - textureEditUv1;
	glm::vec2 uv1(0,0);
	glm::vec2 uv4(1,1);

	glm::vec2 uv2(uv4.x, uv1.y);
	glm::vec2 uv3(uv1.x, uv4.y);

	if (textureBrushFlipD)
		std::swap(uv1, uv4); // different from texturebrush window due to flipped UV

	if (textureBrushFlipH)
	{
		uv1.x = 1 - uv1.x;
		uv2.x = 1 - uv2.x;
		uv3.x = 1 - uv3.x;
		uv4.x = 1 - uv4.x;
	}
	if (textureBrushFlipV)
	{
		uv1.y = 1 - uv1.y;
		uv2.y = 1 - uv2.y;
		uv3.y = 1 - uv3.y;
		uv4.y = 1 - uv4.y;
	}
	glm::vec2 xInc = (uv2 - uv1) * uvSize / (float)textureBrushWidth;
	glm::vec2 yInc = (uv3 - uv1) * uvSize / (float)textureBrushHeight;
	if (textureBrushFlipD)
	{
		if (textureBrushFlipV ^ textureBrushFlipH)
		{
			xInc = (uv1 - uv2) * uvSize / (float)textureBrushHeight;
			yInc = (uv1 - uv3) * uvSize / (float)textureBrushWidth;
		}
		if (textureBrushFlipH == textureBrushFlipV)
		{
			xInc = (uv2 - uv1) * uvSize / (float)textureBrushHeight;
			yInc = (uv3 - uv1) * uvSize / (float)textureBrushWidth;

		}
		std::swap(xInc, yInc);
		std::swap(xInc.x, xInc.y);
		std::swap(yInc.x, yInc.y);
	}

	glm::vec2 uvStart = uv1 + textureEditUv1.x * (uv2 - uv1) + textureEditUv1.y * (uv3 - uv1);

	ImGui::Begin("Statusbar");
	ImGui::Text("UV1 %.1f,%.1f  UV2 %.1f,%.1f  UV3 %.1f,%.1f  UV4 %.1f,%.1f", uv1.x, uv1.y, uv2.x, uv2.y, uv3.x, uv3.y, uv4.x, uv4.y);
	ImGui::SameLine();
	ImGui::Text("xInc %.3f,%.3f  yInc %.3f,%.3f ", xInc.x, xInc.y, yInc.x, yInc.y);
	ImGui::SameLine();
	ImGui::Text("uvStart %.3f,%.3f", uvStart.x, uvStart.y);
	ImGui::End();


	glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));
	if (browEdit->textureBrushMode == BrowEdit::TextureBrushMode::Stamp)
	{
		if (browEdit->textureStamp.size() == 0 && !ImGui::GetIO().KeyShift)
		{
			if (mouse3D != glm::vec3(std::numeric_limits<float>::max()) && !(ImGui::IsMouseDown(0) && ImGui::GetIO().KeyShift) && textureBrushWidth > 0 && textureBrushHeight > 0)
			{
				//#define UVDEBUG
#ifdef UVDEBUG
				ImGui::Begin("Debug");
				ImGui::BeginTable("bla", textureBrushWidth, ImGuiTableFlags_Borders);
#endif

				std::vector<VertexP3T2N3> verts;
				float dist = 0.002f * cameraDistance;

				glm::ivec2 topleft = tileHovered - glm::ivec2(textureBrushWidth / 2, textureBrushHeight / 2);
				for (int x = 0; x < textureBrushWidth; x++)
				{
#ifdef UVDEBUG
					ImGui::TableNextRow();
#endif
					for (int y = 0; y < textureBrushHeight; y++)
					{
						if (topleft.x + x >= gnd->width || topleft.x + x < 0 || topleft.y + y >= gnd->height || topleft.y + y < 0)
							continue;
						auto cube = gnd->cubes[topleft.x + x][topleft.y + y];

						glm::vec2 uv1_ = uvStart + xInc * (float)x + yInc * (float)y;
						glm::vec2 uv2_ = uv1_ + xInc;
						glm::vec2 uv3_ = uv1_ + yInc;
						glm::vec2 uv4_ = uv1_ + xInc + yInc;

						uv1_.y = 1 - uv1_.y;
						uv2_.y = 1 - uv2_.y;
						uv3_.y = 1 - uv3_.y;
						uv4_.y = 1 - uv4_.y;

#ifdef UVDEBUG
						ImGui::TableNextColumn();
						ImGui::Text("%.2f, %.2f", uv1_.x, uv1_.y);
						ImGui::SameLine(95);
						ImGui::Text("%.2f, %.2f", uv2_.x, uv2_.y);
						ImGui::Text("%.2f, %.2f", uv3_.x, uv3_.y);
						ImGui::SameLine(95);
						ImGui::Text("%.2f, %.2f", uv4_.x, uv4_.y);
						ImGui::Spacing();
						ImGui::Spacing();
#endif

						verts.push_back(VertexP3T2N3(glm::vec3(10 * (topleft.x + x), -cube->h3 + dist, 10 * gnd->height - 10 * (topleft.y + y)), uv3_, cube->normals[2]));
						verts.push_back(VertexP3T2N3(glm::vec3(10 * (topleft.x + x) + 10, -cube->h2 + dist, 10 * gnd->height - 10 * (topleft.y + y) + 10), uv2_, cube->normals[1]));
						verts.push_back(VertexP3T2N3(glm::vec3(10 * (topleft.x + x) + 10, -cube->h4 + dist, 10 * gnd->height - 10 * (topleft.y + y)), uv4_, cube->normals[3]));

						verts.push_back(VertexP3T2N3(glm::vec3(10 * (topleft.x + x), -cube->h1 + dist, 10 * gnd->height - 10 * (topleft.y + y) + 10), uv1_, cube->normals[0]));
						verts.push_back(VertexP3T2N3(glm::vec3(10 * (topleft.x + x), -cube->h3 + dist, 10 * gnd->height - 10 * (topleft.y + y)), uv3_, cube->normals[2]));
						verts.push_back(VertexP3T2N3(glm::vec3(10 * (topleft.x + x) + 10, -cube->h2 + dist, 10 * gnd->height - 10 * (topleft.y + y) + 10), uv2_, cube->normals[1]));
					}
				}
#ifdef UVDEBUG
				ImGui::EndTable();
#endif
				if (verts.size() > 0)
				{
					glEnableVertexAttribArray(0);
					glEnableVertexAttribArray(1);
					glEnableVertexAttribArray(2);
					glDisableVertexAttribArray(3);
					glDisableVertexAttribArray(4); //TODO: vao
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
					glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
					glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);
					gndRenderer->textures[textureSelected]->bind();
					simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 1, 0.5f));
					simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
					simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
					glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
					simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 0.5f);
					simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
				}
#ifdef UVDEBUG
				ImGui::End();
#endif
			}
		}
		else if (!ImGui::GetIO().KeyShift && browEdit->textureStampMap && browEdit->textureStampMap->rootNode) //imagestamp
		{
			glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));
			glm::ivec2 topleft = tileHovered - glm::ivec2(browEdit->textureStamp.size() / 2, browEdit->textureStamp[0].size() / 2);
			float dist = 0.002f * cameraDistance;
			std::map<int, std::vector<VertexP3T2N3>> verts;
			for (auto xx = 0; xx < browEdit->textureStamp.size(); xx++)
			{
				for (auto yy = 0; yy < browEdit->textureStamp[xx].size(); yy++)
				{
					int x = topleft.x + xx;
					int y = topleft.y + yy;
					if (x < 0 || x >= gnd->width || y < 0 || y >= gnd->height)
						continue;
					auto tile = browEdit->textureStamp[xx][yy];
					if (!tile)
						continue;
					auto cube = gnd->cubes[x][y];
					verts[tile->textureIndex].push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h3 + dist, 10 * gnd->height - 10 * y), tile->v3, cube->normals[2]));
					verts[tile->textureIndex].push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h1 + dist, 10 * gnd->height - 10 * y + 10), tile->v1, cube->normals[0]));
					verts[tile->textureIndex].push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * y), tile->v4, cube->normals[3]));

					verts[tile->textureIndex].push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h1 + dist, 10 * gnd->height - 10 * y + 10), tile->v1, cube->normals[0]));
					verts[tile->textureIndex].push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * y), tile->v4, cube->normals[3]));
					verts[tile->textureIndex].push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * y + 10), tile->v2, cube->normals[1]));
				}
			}
			simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 1, 0.5f));
			auto mapGndRenderer = browEdit->textureStampMap->rootNode->getComponent<GndRenderer>();
			for (auto texVerts : verts)
			{
				mapGndRenderer->textures[texVerts.first]->bind();
				glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), texVerts.second[0].data);
				glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), texVerts.second[0].data + 3);
				glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), texVerts.second[0].data + 5);
				glDrawArrays(GL_TRIANGLES, 0, (int)texVerts.second.size());
			}
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 0.5f);
		}

		if (hovered && mouse3D != glm::vec3(std::numeric_limits<float>::max()))
		{
			if (deleteTiles && browEdit->textureStamp.size() == 0)
			{
				auto ga = new GroupAction();
				deleteTiles = false;

				glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));
				glm::ivec2 topleft = tileHovered - glm::ivec2(textureBrushWidth / 2, textureBrushHeight / 2);
				
				for (int x = 0; x < textureBrushWidth; x++)
				{
					for (int y = 0; y < textureBrushHeight; y++)
					{
						if (topleft.x + x >= gnd->width || topleft.x + x < 0 || topleft.y + y >= gnd->height || topleft.y + y < 0)
							continue;
						auto cube = gnd->cubes[topleft.x + x][topleft.y + y];
						ga->addAction(new CubeTileChangeAction(glm::ivec2(topleft.x + x, topleft.y + y), cube, -1, cube->tileFront, cube->tileSide));
						cube->tileUp = -1;
					}
				}

				map->doAction(ga, browEdit);
			}
			if (ImGui::IsMouseDown(0) && ImGui::GetIO().KeyShift)
			{//dragging mouse to show preview
				if (!mouseDown)
					mouseDragStart = mouse3D;
				mouseDown = true;
				auto mouseDragEnd = mouse3D;
				std::vector<VertexP3T2N3> verts;
				float dist = 0.002f * cameraDistance;

				int tileMinX = (int)glm::floor(glm::min(mouseDragStart.x, mouseDragEnd.x) / 10);
				int tileMaxX = (int)glm::ceil(glm::max(mouseDragStart.x, mouseDragEnd.x) / 10);

				int tileMaxY = gnd->height - (int)glm::floor(glm::min(mouseDragStart.z, mouseDragEnd.z) / 10) + 1;
				int tileMinY = gnd->height - (int)glm::ceil(glm::max(mouseDragStart.z, mouseDragEnd.z) / 10) + 1;


				ImGui::Begin("Statusbar");
				ImGui::Text("Selection: (%d,%d) - (%d,%d)", tileMinX, tileMinY, tileMaxX, tileMaxY);
				ImGui::End();

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
				if (verts.size() > 0)
				{
					glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
					glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
					glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);
					simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0, 0, 0.5f));
					glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
				}
			}
			else if (ImGui::IsMouseReleased(0) && mouseDown)
			{ //release mouse to create texturestamp
				mouseDown = false;
				for (auto r : browEdit->textureStamp)
					for (auto rr : r)
						delete rr;
				browEdit->textureStamp.clear();
				if (ImLengthSqr(ImGui::GetMouseDragDelta(0)) > 3)
				{
					auto mouseDragEnd = mouse3D;
					mouseDown = false;
					int tileMinX = (int)glm::floor(glm::min(mouseDragStart.x, mouseDragEnd.x) / 10);
					int tileMaxX = (int)glm::ceil(glm::max(mouseDragStart.x, mouseDragEnd.x) / 10);

					int tileMaxY = gnd->height - (int)glm::floor(glm::min(mouseDragStart.z, mouseDragEnd.z) / 10) + 1;
					int tileMinY = gnd->height - (int)glm::ceil(glm::max(mouseDragStart.z, mouseDragEnd.z) / 10) + 1;

					browEdit->textureStampMap = map;

					if (tileMinX >= 0 && tileMaxX < gnd->width + 1 && tileMinY >= 0 && tileMaxY < gnd->height + 1)
						for (int x = tileMinX; x < tileMaxX; x++)
						{
							std::vector<Gnd::Tile*> col;
							for (int y = tileMinY; y < tileMaxY; y++)
							{
								auto cube = gnd->cubes[x][y];
								if (cube->tileUp == -1)
									col.push_back(nullptr);
								else
									col.push_back(new Gnd::Tile(*gnd->tiles[cube->tileUp]));
							}
							browEdit->textureStamp.push_back(col);
						}
				}

			}
			else if (ImGui::IsMouseClicked(0) && !ImGui::GetIO().KeyShift)
			{
				if (browEdit->textureStamp.size() == 0)
				{
					auto ga = new GroupAction();

					glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));
					glm::ivec2 topleft = tileHovered - glm::ivec2(textureBrushWidth / 2, textureBrushHeight / 2);
					int id = (int)gnd->tiles.size();
					for (int x = 0; x < textureBrushWidth; x++)
					{
						for (int y = 0; y < textureBrushHeight; y++)
						{
							if (topleft.x + x >= gnd->width || topleft.x + x < 0 || topleft.y + y >= gnd->height || topleft.y + y < 0)
								continue;
							auto cube = gnd->cubes[topleft.x + x][topleft.y + y];

							Gnd::Tile* t = new Gnd::Tile();

							t->v1 = uvStart + xInc * (float)x + yInc * (float)y;
							t->v2 = t->v1 + xInc;
							t->v3 = t->v1 + yInc;
							t->v4 = t->v1 + xInc + yInc;

							for (int i = 0; i < 4; i++)
								t->texCoords[i].y = 1 - t->texCoords[i].y;

							t->color = glm::ivec4(255, 255, 255, 255);
							t->textureIndex = textureSelected;
							t->lightmapIndex = 0;
							if (textureBrushKeepShadow && cube->tileUp != -1)
								t->lightmapIndex = gnd->tiles[cube->tileUp]->lightmapIndex;
							if (textureBrushKeepColor && cube->tileUp != -1)
								t->color = gnd->tiles[cube->tileUp]->color;

							ga->addAction(new TileNewAction(t));
							ga->addAction(new CubeTileChangeAction(glm::ivec2(topleft.x + x, topleft.y + y), cube, id, cube->tileFront, cube->tileSide));
							cube->tileUp = id;
							id++;
						}
					}
					textureGridDirty = true;
					map->doAction(ga, browEdit);
				}
				else
				{//do texture stamping
					auto ga = new GroupAction();
					std::map<int, int> textureStampLookup;
					glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));
					glm::ivec2 topleft = tileHovered - glm::ivec2(browEdit->textureStamp.size() / 2, browEdit->textureStamp[0].size() / 2);
					int id = (int)gnd->tiles.size();

					auto stampGnd = browEdit->textureStampMap->rootNode->getComponent<Gnd>();
					auto stampGndRenderer = browEdit->textureStampMap->rootNode->getComponent<GndRenderer>();

					for (auto xx = 0; xx < browEdit->textureStamp.size(); xx++)
					{
						for (int yy = 0; yy < browEdit->textureStamp[xx].size(); yy++)
						{
							int x = topleft.x + xx;
							int y = topleft.y + yy;
							if (x < 0 || x >= gnd->width || y < 0 || y >= gnd->height)
								continue;
							auto cube = gnd->cubes[x][y];
							if (!browEdit->textureStamp[xx][yy])
								continue;
							Gnd::Tile* t = new Gnd::Tile(*browEdit->textureStamp[xx][yy]);
							if (textureBrushKeepShadow && cube->tileUp != -1)
								t->lightmapIndex = gnd->tiles[cube->tileUp]->lightmapIndex;
							if (textureBrushKeepColor && cube->tileUp != -1)
								t->color = gnd->tiles[cube->tileUp]->color;

							if (browEdit->textureStampMap != map)
							{
								if (textureStampLookup.find(t->textureIndex) == textureStampLookup.end())
								{
									auto tx = stampGnd->textures[t->textureIndex]->file;
									for (auto i = 0; i < gnd->textures.size(); i++)
										if (gnd->textures[i]->file == tx)
											textureStampLookup[t->textureIndex] = i;
									if (textureStampLookup.find(t->textureIndex) == textureStampLookup.end())
									{
										textureStampLookup[t->textureIndex] = (int)gnd->textures.size();
										gnd->textures.push_back(new Gnd::Texture(tx, tx));
										gndRenderer->textures.push_back(util::ResourceManager<gl::Texture>::load("data\\texture\\" + tx));
										ga->addAction(new GndTextureAddAction(tx));
									}
								}
								t->textureIndex = textureStampLookup[t->textureIndex];
							}
							ga->addAction(new TileNewAction(t));
							ga->addAction(new CubeTileChangeAction(glm::ivec2(x, y), cube, id, cube->tileFront, cube->tileSide));
							cube->tileUp = id;
							id++;
						}
					}
					textureGridDirty = true;
					map->doAction(ga, browEdit);
				}
			}

		}
	}
	else if (browEdit->textureBrushMode == BrowEdit::TextureBrushMode::Select)
	{
		auto mouse3D = gnd->rayCast(mouseRay, viewEmptyTiles);
		glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));

		//draw selection
		if (map->tileSelection.size() > 0)
		{
			std::vector<VertexP3T2N3> verts;
			float dist = 0.002f * cameraDistance;
			for (auto& tile : map->tileSelection)
			{
				auto cube = gnd->cubes[tile.x][tile.y];

				glm::vec2 v1 = uvStart + xInc * (float)((tile.x + browEdit->textureFillOffset.x)%textureBrushWidth) + yInc * (float)((tile.y + browEdit->textureFillOffset.y)%textureBrushHeight);
				glm::vec2 v2 = v1 + xInc;
				glm::vec2 v3 = v1 + yInc;
				glm::vec2 v4 = v1 + xInc + yInc;

				v1.y = 1.0f - v1.y;
				v2.y = 1.0f - v2.y;
				v3.y = 1.0f - v3.y;
				v4.y = 1.0f - v4.y;

				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, -cube->h3 + dist, 10 * gnd->height - 10 * tile.y), v3, cube->normals[2]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * tile.y + 10), v2, cube->normals[1]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * tile.y), v4, cube->normals[3]));

				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, -cube->h1 + dist, 10 * gnd->height - 10 * tile.y + 10), v1, cube->normals[0]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, -cube->h3 + dist, 10 * gnd->height - 10 * tile.y), v3, cube->normals[2]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * tile.y + 10), v2, cube->normals[1]));
			}
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glDisableVertexAttribArray(3);
			glDisableVertexAttribArray(4); //TODO: vao
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
			glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);


			gndRenderer->textures[textureSelected]->bind();
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1.0f, 1.0f, 0.5f));
			simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
			glDepthMask(0);
			glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0.0f, 0.0f, 0.5f));
			simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
			glDepthMask(1);
			glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		}


		if (hovered)
		{
			if (ImGui::IsMouseDown(0))
			{
				if (!mouseDown)
					mouseDragStart = mouse3D;
				mouseDown = true;
			}

			if (ImGui::IsMouseReleased(0))
			{
				mouseDown = false;
				if (ImLengthSqr(ImGui::GetMouseDragDelta(0)) < 3)
				{
					auto mouseDragEnd = mouse3D;

					std::vector<glm::ivec2> newSelection;
					if (ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl)
						newSelection = map->tileSelection;

					if (std::find(map->tileSelection.begin(), map->tileSelection.end(), tileHovered) != map->tileSelection.end())
					{
						auto ga = new GroupAction();
						int id = (int)gnd->tiles.size();
						for (auto& tile : map->tileSelection)
						{
							auto cube = gnd->cubes[tile.x][tile.y];

							auto t = new Gnd::Tile();

							t->v1 = uvStart + xInc * (float)((tile.x + browEdit->textureFillOffset.x) % textureBrushWidth) + yInc * (float)((tile.y + browEdit->textureFillOffset.y) % textureBrushHeight);
							t->v2 = t->v1 + xInc;
							t->v3 = t->v1 + yInc;
							t->v4 = t->v1 + xInc + yInc;

							t->v1.y = 1.0f - t->v1.y;
							t->v2.y = 1.0f - t->v2.y;
							t->v3.y = 1.0f - t->v3.y;
							t->v4.y = 1.0f - t->v4.y;


							t->color = glm::ivec4(255, 255, 255, 255);
							t->lightmapIndex = 0;
							t->textureIndex = textureSelected;

							if (cube->tileUp != -1)
							{
								if (textureBrushKeepColor)
									t->color = gnd->tiles[cube->tileUp]->color;
								if (textureBrushKeepShadow)
									t->lightmapIndex = gnd->tiles[cube->tileUp]->lightmapIndex;
							}
							
							gndRenderer->setChunkDirty(tile.x, tile.y);

							ga->addAction(new TileNewAction(t));
							ga->addAction(new CubeTileChangeAction(tile, cube, id, cube->tileFront, cube->tileSide));
							id++;
						}
						map->doAction(ga, browEdit);

						map->tileSelection.clear();
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
						if (map->tileSelection != newSelection)
							map->doAction(new TileSelectAction(map, newSelection), browEdit);
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
						if (map->tileSelection != newSelection)
							map->doAction(new TileSelectAction(map, newSelection), browEdit);
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
									if (std::find(newSelection.begin(), newSelection.end(), t) != newSelection.end())
										newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [&](const glm::ivec2& el) { return el.x == t.x && el.y == t.y; }));
								}
								else if (std::find(newSelection.begin(), newSelection.end(), t) == newSelection.end())
									newSelection.push_back(t);
							}
							if (map->tileSelection != newSelection)
								map->doAction(new TileSelectAction(map, newSelection), browEdit);
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
									if (std::find(newSelection.begin(), newSelection.end(), t) != newSelection.end())
										newSelection.erase(std::remove_if(newSelection.begin(), newSelection.end(), [&](const glm::ivec2& el) { return el.x == t.x && el.y == t.y; }));
								}
								else if (std::find(newSelection.begin(), newSelection.end(), t) == newSelection.end())
									newSelection.push_back(t);
							}
							if (map->tileSelection != newSelection)
								map->doAction(new TileSelectAction(map, newSelection), browEdit);
						}
					}



				}
				else
				{
					auto mouseDragEnd = mouse3D;

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
								if (polygon.contains(glm::vec2(x, y)))
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
					


					if (map->tileSelection != newSelection)
					{
						map->doAction(new TileSelectAction(map, newSelection), browEdit);
					}
				}
			}
		}
		if (mouseDown)
		{
			auto mouseDragEnd = mouse3D;
			std::vector<VertexP3T2N3> verts;
			float dist = 0.003f * cameraDistance;

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
			else if (browEdit->selectTool == BrowEdit::SelectTool::Lasso)
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
	else if (browEdit->textureBrushMode == BrowEdit::TextureBrushMode::Fill)
	{
		auto mouse3D = gnd->rayCast(mouseRay, viewEmptyTiles);
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
				if (done.find(pp.x + gnd->width*pp.y) == done.end())
				{
					done[pp.x + gnd->width*pp.y] = true;
					queue.push(pp);
				}
			}
			if (gnd->inMap(p) && std::find(tilesToFill.begin(), tilesToFill.end(), p) == tilesToFill.end())
				tilesToFill.push_back(p);
			c++;
		}
		ImGui::Begin("Statusbar");
		ImGui::SameLine();
		ImGui::Text("Tiles gone through: %d", c);
		ImGui::End();


		std::vector<VertexP3T2N3> verts;
		float dist = 0.002f * cameraDistance;
		for (auto& tile : tilesToFill)
		{
			auto cube = gnd->cubes[tile.x][tile.y];

			glm::vec2 v1 = uvStart + xInc * (float)((tile.x + browEdit->textureFillOffset.x) % textureBrushWidth) + yInc * (float)((tile.y + browEdit->textureFillOffset.y) % textureBrushHeight);
			glm::vec2 v2 = v1 + xInc;
			glm::vec2 v3 = v1 + yInc;
			glm::vec2 v4 = v1 + xInc + yInc;

			v1.y = 1.0f - v1.y;
			v2.y = 1.0f - v2.y;
			v3.y = 1.0f - v3.y;
			v4.y = 1.0f - v4.y;

			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, -cube->h3 + dist, 10 * gnd->height - 10 * tile.y), v3, cube->normals[2]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * tile.y + 10), v2, cube->normals[1]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * tile.y), v4, cube->normals[3]));

			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, -cube->h1 + dist, 10 * gnd->height - 10 * tile.y + 10), v1, cube->normals[0]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, -cube->h3 + dist, 10 * gnd->height - 10 * tile.y), v3, cube->normals[2]));
			verts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * tile.y + 10), v2, cube->normals[1]));
		}
		if (verts.size() > 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			glDisableVertexAttribArray(3);
			glDisableVertexAttribArray(4); //TODO: vao
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
			glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);

			gndRenderer->textures[textureSelected]->bind();
			simpleShader->use();
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1.0f, 1.0f, 0.5f));
			simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::shadeType, 1);
			glDepthMask(0);
			glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0.0f, 0.0f, 0.5f));
			simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::shadeType, 1);
			glDepthMask(1);
			glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		if (hovered && ImGui::IsMouseClicked(0) && tilesToFill.size() > 0)
		{
			auto ga = new GroupAction();
			int id = (int)gnd->tiles.size();
			for (auto& tile : tilesToFill)
			{
				auto cube = gnd->cubes[tile.x][tile.y];

				auto t = new Gnd::Tile();

				t->v1 = uvStart + xInc * (float)((tile.x + browEdit->textureFillOffset.x) % textureBrushWidth) + yInc * (float)((tile.y + browEdit->textureFillOffset.y) % textureBrushHeight);
				t->v2 = t->v1 + xInc;
				t->v3 = t->v1 + yInc;
				t->v4 = t->v1 + xInc + yInc;

				t->v1.y = 1.0f - t->v1.y;
				t->v2.y = 1.0f - t->v2.y;
				t->v3.y = 1.0f - t->v3.y;
				t->v4.y = 1.0f - t->v4.y;

				t->color = glm::ivec4(255, 255, 255, 255);
				t->lightmapIndex = 0;
				t->textureIndex = textureSelected;

				if (cube->tileUp != -1)
				{
					if(textureBrushKeepColor)
						t->color = gnd->tiles[cube->tileUp]->color;
					if(textureBrushKeepShadow)
						t->lightmapIndex = gnd->tiles[cube->tileUp]->lightmapIndex;
				}

				gndRenderer->setChunkDirty(tile.x, tile.y);

				ga->addAction(new TileNewAction(t));
				ga->addAction(new CubeTileChangeAction(tile, cube, id, cube->tileFront, cube->tileSide));
				id++;
			}
			map->doAction(ga, browEdit);
		}

	
	}
	else if (browEdit->textureBrushMode == BrowEdit::TextureBrushMode::Dropper)
	{
		browEdit->cursor = browEdit->dropperCursor;
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			if (gnd->inMap(tileHovered))
			{
				if (gnd->cubes[tileHovered.x][tileHovered.y]->tileUp != -1)
				{
					textureSelected = gnd->tiles[gnd->cubes[tileHovered.x][tileHovered.y]->tileUp]->textureIndex;
					browEdit->textureBrushMode = browEdit->brushModeBeforeDropper;
				}
			}
		}
	}

	deleteTiles = false;
	fbo->unbind();

}


















void MapView::buildTextureGrid()
{
	std::vector<VertexP3T2> verts;
	auto gnd = map->rootNode->getComponent<Gnd>();
	for (int x = 1; x < gnd->width - 1; x++)
	{
		for (int y = 1; y < gnd->height - 1; y++)
		{
			if (textureGridSmart)
			{
				{
					Gnd::Tile* t1 = NULL;
					Gnd::Tile* t2 = NULL;
					if (gnd->cubes[x][y]->tileUp != -1)
						t1 = gnd->tiles[gnd->cubes[x][y]->tileUp];
					if (gnd->cubes[x + 1][y]->tileUp != -1)
						t2 = gnd->tiles[gnd->cubes[x + 1][y]->tileUp];

					bool drawLine = false;
					if ((t1 == NULL) != (t2 == NULL)) // NULL next to a tile
						drawLine = true;
					else if (t1 == NULL)
						drawLine = false; // both tiles are NULL
					else if (t1->textureIndex != t2->textureIndex) // 2 different textures
						drawLine = true;
					else if (glm::distance(t1->v2, t2->v1) > 0.1 || glm::distance(t1->v4, t2->v3) > 0.1)
						drawLine = true;

					if (drawLine)
					{
						verts.push_back(VertexP3T2(glm::vec3(10 * x + 10, -gnd->cubes[x][y]->h4 + 0.1f, 10 * gnd->height - 10 * y), glm::vec2()));
						verts.push_back(VertexP3T2(glm::vec3(10 * x + 10, -gnd->cubes[x][y]->h2 + 0.1f, 10 * gnd->height - 10 * y + 10), glm::vec2()));
					}
				}

				{
					Gnd::Tile* t1 = NULL;
					Gnd::Tile* t2 = NULL;
					if (gnd->cubes[x][y]->tileUp != -1)
						t1 = gnd->tiles[gnd->cubes[x][y]->tileUp];
					if (gnd->cubes[x - 1][y]->tileUp != -1)
						t2 = gnd->tiles[gnd->cubes[x - 1][y]->tileUp];

					bool drawLine = false;
					if ((t1 == NULL) != (t2 == NULL)) // NULL next to a tile
						drawLine = true;
					else if (t1 == NULL)
						drawLine = false; // both tiles are NULL
					else if (t1->textureIndex != t2->textureIndex) // 2 different textures
						drawLine = true;
					else if (glm::distance(t1->v1, t2->v2) > 0.1f || glm::distance(t1->v3, t2->v4) > 0.1f)
						drawLine = true;

					if (drawLine)
					{
						verts.push_back(VertexP3T2(glm::vec3(10 * x, -gnd->cubes[x][y]->h3 + 0.1f, 10 * gnd->height - 10 * y), glm::vec2()));
						verts.push_back(VertexP3T2(glm::vec3(10 * x, -gnd->cubes[x][y]->h1 + 0.1f, 10 * gnd->height - 10 * y + 10), glm::vec2()));
					}
				}

				{
					Gnd::Tile* t1 = NULL;
					Gnd::Tile* t2 = NULL;
					if (gnd->cubes[x][y]->tileUp != -1)
						t1 = gnd->tiles[gnd->cubes[x][y]->tileUp];
					if (gnd->cubes[x][y + 1]->tileUp != -1)
						t2 = gnd->tiles[gnd->cubes[x][y + 1]->tileUp];

					bool drawLine = false;
					if ((t1 == NULL) != (t2 == NULL)) // NULL next to a tile
						drawLine = true;
					else if (t1 == NULL)
						drawLine = false; // both tiles are NULL
					else if (t1->textureIndex != t2->textureIndex) // 2 different textures
						drawLine = true;
					else if (glm::distance(t1->v3, t2->v1) > 0.1 || glm::distance(t1->v4, t2->v2) > 0.1)
						drawLine = true;

					if (drawLine)
					{
						verts.push_back(VertexP3T2(glm::vec3(10 * x, -gnd->cubes[x][y]->h3 + 0.1f, 10 * gnd->height - 10 * y), glm::vec2()));
						verts.push_back(VertexP3T2(glm::vec3(10 * x + 10, -gnd->cubes[x][y]->h4 + 0.1f, 10 * gnd->height - 10 * y), glm::vec2()));
					}
				}

				{
					Gnd::Tile* t1 = NULL;
					Gnd::Tile* t2 = NULL;
					if (gnd->cubes[x][y]->tileUp != -1)
						t1 = gnd->tiles[gnd->cubes[x][y]->tileUp];
					if (gnd->cubes[x][y - 1]->tileUp != -1)
						t2 = gnd->tiles[gnd->cubes[x][y - 1]->tileUp];

					bool drawLine = false;
					if ((t1 == NULL) != (t2 == NULL)) // NULL next to a tile
						drawLine = true;
					else if (t1 == NULL)
						drawLine = false; // both tiles are NULL
					else if (t1->textureIndex != t2->textureIndex) // 2 different textures
						drawLine = true;
					else if (glm::distance(t1->v1, t2->v3) > 0.1f || glm::distance(t1->v2, t2->v4) > 0.1f)
						drawLine = true;

					if (drawLine)
					{
						verts.push_back(VertexP3T2(glm::vec3(10 * x, -gnd->cubes[x][y]->h1 + 0.1f, 10 * gnd->height - 10 * y + 10), glm::vec2()));
						verts.push_back(VertexP3T2(glm::vec3(10 * x + 10, -gnd->cubes[x][y]->h2 + 0.1f, 10 * gnd->height - 10 * y + 10), glm::vec2()));
					}
				}
			}
			else
			{
				auto cube = gnd->cubes[x][y];
				VertexP3T2 v1(glm::vec3(10 * x, -cube->h3+0.1f, 10 * gnd->height - 10 * y), glm::vec2(1));
				VertexP3T2 v2(glm::vec3(10 * x + 10, -cube->h4 + 0.1f, 10 * gnd->height - 10 * y), glm::vec2(1));
				VertexP3T2 v3(glm::vec3(10 * x, -cube->h1 + 0.1f, 10 * gnd->height - 10 * y + 10), glm::vec2(1));
				VertexP3T2 v4(glm::vec3(10 * x + 10, -cube->h2 + 0.1f, 10 * gnd->height - 10 * y + 10), glm::vec2(1));

				verts.push_back(v1);
				verts.push_back(v2);
				verts.push_back(v3);
				verts.push_back(v4);
				verts.push_back(v1);
				verts.push_back(v3);
				verts.push_back(v2);
				verts.push_back(v4);
			}
		}
	}
	textureGridVbo->setData(verts, GL_STATIC_DRAW);
}