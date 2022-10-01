#include <Windows.h>
#include "MapView.h"
#include <browedit/BrowEdit.h>
#include <browedit/shaders/SimpleShader.h>
#include <browedit/gl/FBO.h>
#include <browedit/Map.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/Node.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/TilePropertyChangeAction.h>
#include <glm/gtc/type_ptr.hpp>


void MapView::postRenderColorMode(BrowEdit* browEdit)
{
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

	ImGui::Begin("Statusbar");
	ImGui::SetNextItemWidth(100.0f);
	ImGui::Text("Cursor: %d,%d", tileHovered.x, tileHovered.y);
	ImGui::SameLine();
	ImGui::End();

	if (hovered && gnd->inMap(tileHovered))
	{
		std::vector<VertexP3T2N3> verts;
		float dist = 0.002f * cameraDistance;
		for (int x = tileHovered.x - browEdit->colorEditBrushSize; x <= tileHovered.x + browEdit->colorEditBrushSize; x++)
		{
			for (int y = tileHovered.y - browEdit->colorEditBrushSize; y <= tileHovered.y + browEdit->colorEditBrushSize; y++)
			{
				if (!gnd->inMap(glm::ivec2(x, y)))
					continue;

				const auto cube = gnd->cubes[x][y];

				verts.push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h1 + dist, 10 * gnd->height - 10 * y + 10), glm::vec2(0), cube->normals[0]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * y + 10), glm::vec2(0), cube->normals[1]));

				verts.push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h2 + dist, 10 * gnd->height - 10 * y + 10), glm::vec2(0), cube->normals[1]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * y), glm::vec2(0), cube->normals[3]));

				verts.push_back(VertexP3T2N3(glm::vec3(10 * x + 10, -cube->h4 + dist, 10 * gnd->height - 10 * y), glm::vec2(0), cube->normals[3]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h3 + dist, 10 * gnd->height - 10 * y), glm::vec2(0), cube->normals[2]));

				verts.push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h3 + dist, 10 * gnd->height - 10 * y), glm::vec2(0), cube->normals[2]));
				verts.push_back(VertexP3T2N3(glm::vec3(10 * x, -cube->h1 + dist, 10 * gnd->height - 10 * y + 10), glm::vec2(0), cube->normals[0]));
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

			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0, 1, 0, 1));
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
			glDrawArrays(GL_LINES, 0, (int)verts.size());
		}
	


		static double lastTime = ImGui::GetTime();

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::GetTime() - lastTime > browEdit->colorEditDelay))
		{
			lastTime = ImGui::GetTime();
			auto ga = new GroupAction();
			for (int x = tileHovered.x - browEdit->colorEditBrushSize; x <= tileHovered.x + browEdit->colorEditBrushSize; x++)
			{
				for (int y = tileHovered.y - browEdit->colorEditBrushSize; y <= tileHovered.y + browEdit->colorEditBrushSize; y++)
				{
					if (gnd->inMap(glm::ivec2(x,y)) && gnd->cubes[x][y]->tileUp != -1)
					{
						float strength = 1;
						if (browEdit->colorEditBrushSize > 0)
						{
							float dist = glm::length(glm::vec2(x - tileHovered.x, y - tileHovered.y)) / browEdit->colorEditBrushSize;
							strength = glm::clamp((1 - dist), 0.0f, 1.0f);
							strength = glm::clamp(strength + browEdit->colorEditBrushHardness, 0.0f, 1.0f);
						}
						auto tile = gnd->tiles[gnd->cubes[x][y]->tileUp];
						auto oldColor = tile->color;
						tile->color = glm::mix(tile->color, glm::ivec4(glm::vec3(browEdit->colorEditBrushColor) * 255.0f, 255), browEdit->colorEditBrushColor.a * strength);
						ga->addAction(new TileChangeAction<glm::ivec4>(tile, &tile->color, oldColor, "Change color"));
					}
				}
			}
			map->doAction(ga, browEdit);
		}

	}

	glDepthMask(1);
	glDisable(GL_BLEND);
	fbo->unbind();
}