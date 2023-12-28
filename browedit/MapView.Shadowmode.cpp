#include <Windows.h>
#include "MapView.h"
#include <browedit/BrowEdit.h>
#include <browedit/shaders/SimpleShader.h>
#include <browedit/gl/FBO.h>
#include <browedit/Map.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/Node.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/TilePropertyChangeAction.h>
#include <browedit/actions/TileNewAction.h>
#include <browedit/actions/LightmapNewAction.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <unordered_set>

extern bool shadowDropperEnabled;

void MapView::postRenderShadowMode(BrowEdit* browEdit)
{
	auto rsw = map->rootNode->getComponent<Rsw>();
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

	auto mouse3D = gnd->rayCast(mouseRay, viewEmptyTiles);
	glm::ivec2 tileHovered((int)glm::floor(mouse3D.x / 10), (gnd->height - (int)glm::floor(mouse3D.z) / 10));
	glm::ivec2 subTileHovered((int)glm::floor(glm::fract(mouse3D.x / 10) * gnd->lightmapWidth), gnd->lightmapHeight - (int)glm::floor(glm::fract(mouse3D.z / 10) * gnd->lightmapHeight));
	glm::ivec2 shadowHoveredOffset((int)glm::floor(mouse3D.x / 10 * gnd->lightmapWidth), (int)((gnd->height - (mouse3D.z - 10) / 10) * gnd->lightmapHeight));

	ImGui::Begin("Statusbar");
	ImGui::SetNextItemWidth(100.0f);
	ImGui::Text("Cursor: %d,%d", tileHovered.x, tileHovered.y);
	ImGui::SameLine();
	ImGui::End();

	//if(hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left))
	//	refColor = browEdit->colorEditBrushColor;

	if (hovered && gnd->inMap(tileHovered))
	{
		if (shadowDropperEnabled && gnd->cubes[tileHovered.x][tileHovered.y]->tileUp != -1)
		{
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				browEdit->shadowEditBrushAlpha = gnd->lightmaps[gnd->tiles[gnd->cubes[tileHovered.x][tileHovered.y]->tileUp]->lightmapIndex]->data[subTileHovered.y * gnd->lightmapWidth + subTileHovered.x];
				shadowDropperEnabled = false;
				browEdit->cursor = nullptr;
			}
		}
		else
		{
			std::vector<VertexP3T2N3> verts;
			float dist = 0.002f * cameraDistance;
			for (int x = 0; x < browEdit->shadowEditBrushSize; x++)
			{
				for (int y = 0; y < browEdit->shadowEditBrushSize; y++)
				{
					int xx = shadowHoveredOffset.x + x - browEdit->shadowEditBrushSize / 2;
					int yy = shadowHoveredOffset.y + y - browEdit->shadowEditBrushSize / 2;

					int cx = xx / gnd->lightmapWidth;
					int cy = yy / gnd->lightmapHeight;

					if (!gnd->inMap(glm::ivec2(cx, cy)))
						continue;

					const auto cube = gnd->cubes[cx][cy];

					float t, th1, th2;
					float xxx = xx * 10.0f / gnd->lightmapWidth;
					float yyy = gnd->height * 10 - yy * 10.0f / gnd->lightmapHeight + 10.0f;
					int subx = xx % gnd->lightmapWidth;
					int suby = yy % gnd->lightmapHeight;

					t = (float)suby / gnd->lightmapWidth;
					th2 = (cube->h4 - cube->h2) * t + cube->h2;
					th1 = (cube->h3 - cube->h1) * t + cube->h1;
					verts.push_back(VertexP3T2N3(glm::vec3(xxx, 
						-(th2 - th1) / gnd->lightmapWidth * subx - th1 + dist,
						yyy), glm::vec2(0), cube->normals[0]));
					verts.push_back(VertexP3T2N3(glm::vec3(xxx + 10.0f / gnd->lightmapWidth, 
						-(th2 - th1) / gnd->lightmapWidth * (subx + 1) - th1 + dist,
						yyy), glm::vec2(0), cube->normals[1]));

					t = (float)(subx + 1) / gnd->lightmapWidth;
					th2 = (cube->h4 - cube->h3) * t + cube->h3;
					th1 = (cube->h2 - cube->h1) * t + cube->h1;
					verts.push_back(VertexP3T2N3(glm::vec3(xxx + 10.0f / gnd->lightmapWidth,
						-(th2 - th1) / gnd->lightmapHeight * suby - th1 + dist,
						yyy), glm::vec2(0), cube->normals[0]));
					verts.push_back(VertexP3T2N3(glm::vec3(xxx + 10.0f / gnd->lightmapWidth,
						-(th2 - th1) / gnd->lightmapHeight * (suby + 1) - th1 + dist,
						yyy - 10.0f / gnd->lightmapHeight), glm::vec2(0), cube->normals[1]));
					
					t = (float)(suby + 1) / gnd->lightmapWidth;
					th2 = (cube->h4 - cube->h2) * t + cube->h2;
					th1 = (cube->h3 - cube->h1) * t + cube->h1;
					verts.push_back(VertexP3T2N3(glm::vec3(xxx,
						-(th2 - th1) / gnd->lightmapWidth * subx - th1 + dist,
						yyy - 10.0f / gnd->lightmapHeight), glm::vec2(0), cube->normals[0]));
					verts.push_back(VertexP3T2N3(glm::vec3(xxx + 10.0f / gnd->lightmapWidth,
						-(th2 - th1) / gnd->lightmapWidth * (subx + 1) - th1 + dist,
						yyy - 10.0f / gnd->lightmapHeight), glm::vec2(0), cube->normals[1]));

					t = (float)subx / gnd->lightmapWidth;
					th2 = (cube->h4 - cube->h3) * t + cube->h3;
					th1 = (cube->h2 - cube->h1) * t + cube->h1;
					verts.push_back(VertexP3T2N3(glm::vec3(xxx,
						-(th2 - th1) / gnd->lightmapHeight * suby - th1 + dist,
						yyy), glm::vec2(0), cube->normals[0]));
					verts.push_back(VertexP3T2N3(glm::vec3(xxx,
						-(th2 - th1) / gnd->lightmapHeight * (suby + 1) - th1 + dist,
						yyy - 10.0f / gnd->lightmapHeight), glm::vec2(0), cube->normals[1]));
				}
			}
			if (verts.size() > 0)
			{
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glEnableVertexAttribArray(2);
				glDisableVertexAttribArray(3);
				glDisableVertexAttribArray(4); //TODO: vao
				glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
				glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
				glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);

				simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0, 1, 0, 0.75f));
				simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
				simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
				glDrawArrays(GL_LINES, 0, (int)verts.size());
			}

			static std::unordered_set<int> tilesProcessed;
			static std::unordered_set<int> subTilesProcessed;
			static GroupAction* ga = nullptr;
			static int lightmapsCount = 0;
			
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				auto shadowProc = [&](int xx, int yy) -> bool {
					if (subTilesProcessed.find(yy << 16 | xx) != subTilesProcessed.end()) {
						return false;
					}
			
					int subx = xx % gnd->lightmapWidth;
					int suby = yy % gnd->lightmapHeight;
					int cx = xx / gnd->lightmapWidth;
					int cy = yy / gnd->lightmapHeight;
			
					if (!gnd->inMap(glm::ivec2(cx, cy)))
						return false;
			
					const auto cube = gnd->cubes[cx][cy];
			
					if (cube->tileUp != -1) {
						int count = 0;
						Gnd::Tile* tile = gnd->tiles[cube->tileUp];
			
						// Happens with undo/redo while painting; though that's just a bandaid, the map will be bugged until a reload
						if (tile->lightmapIndex >= gnd->lightmaps.size())
							return false;
			
						Gnd::Lightmap* lightmap = gnd->lightmaps[tile->lightmapIndex];
			
						if (tilesProcessed.find(cy << 16 | cx) == tilesProcessed.end()) {
							lightmap = new Gnd::Lightmap(*lightmap);
							if (ga == nullptr)
								ga = new GroupAction();
							auto action = new LightmapNewAction(lightmap);
							int oldLightmapIndex = tile->lightmapIndex;
							tile->lightmapIndex = (int)gnd->lightmaps.size();
							lightmapsCount++;
							action->perform(map, browEdit);
							ga->addAction(action);
							ga->addAction(new TileChangeAction<int>(glm::ivec2(cx, cy), tile, &tile->lightmapIndex, oldLightmapIndex, ""));
							tilesProcessed.insert(cy << 16 | cx);
						}
			
						if (ImGui::GetIO().KeyShift)
							lightmap->data[suby * gnd->lightmapWidth + subx] = 255;
						else
							lightmap->data[suby * gnd->lightmapWidth + subx] = (unsigned char)browEdit->shadowEditBrushAlpha;
			
						gndRenderer->setChunkDirty(cx, cy);
						gndRenderer->gndShadowDirty = true;
					}
			
					subTilesProcessed.insert(yy << 16 | xx);
					return true;
				};
			
				for (int x = 0; x < browEdit->shadowEditBrushSize; x++)
				{
					for (int y = 0; y < browEdit->shadowEditBrushSize; y++)
					{
						int xx = shadowHoveredOffset.x + x - browEdit->shadowEditBrushSize / 2;
						int yy = shadowHoveredOffset.y + y - browEdit->shadowEditBrushSize / 2;
						
						if (!browEdit->shadowSmoothEdges) {
							shadowProc(xx, yy);
						}
						else {
							int subx = xx % gnd->lightmapWidth;
							int suby = yy % gnd->lightmapHeight;
			
							if (subx <= 1 || subx >= gnd->lightmapWidth - 2) {
								int extraX = subx <= 1 ? -2 : 2;
			
								if (subx == 0)
									xx++;
								else if (subx == gnd->lightmapWidth - 1)
									xx--;
			
								if (suby == 0)
									yy++;
								else if (suby == gnd->lightmapHeight - 1)
									yy--;
			
								shadowProc(xx + extraX, yy);
			
								if (suby <= 1) {
									shadowProc(xx, yy - 2);
									shadowProc(xx + extraX, yy - 2);
								}
								else if (suby >= gnd->lightmapHeight - 2) {
									shadowProc(xx, yy + 2);
									shadowProc(xx + extraX, yy + 2);
								}
							}
							else if (suby <= 1 || suby >= gnd->lightmapHeight - 2) {
								if (suby == 0)
									yy++;
								else if (suby == gnd->lightmapHeight - 1)
									yy--;
			
								shadowProc(xx, yy + (suby <= 1 ? -2 : 2));
							}
			
							shadowProc(xx, yy);
						}
					}
				}
			}

			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ga != nullptr)
			{
				for (int i = 0; i < lightmapsCount; i++) {
					gnd->lightmaps.pop_back();
				}

				map->doAction(ga, browEdit);
				ga = nullptr;
				tilesProcessed.clear();
				subTilesProcessed.clear();
				lightmapsCount = 0;
			}
		}
	}

	glDepthMask(1);
	glDisable(GL_BLEND);
	fbo->unbind();
}