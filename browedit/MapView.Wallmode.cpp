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
#include <imgui.h>


glm::ivec2 selectionStart;
int selectionSide;
std::vector<glm::ivec3> selectedWalls;
bool previewWall = false; //move this

void MapView::postRenderWallMode(BrowEdit* browEdit)
{
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

	if (gnd->inMap(tileHovered) && gnd->inMap(tileHovered + glm::ivec2(1, 0)) && gnd->inMap(tileHovered + glm::ivec2(0, 1)) && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		auto cube = gnd->cubes[tileHovered.x][tileHovered.y];
		glm::vec3 v1(10 * tileHovered.x + 10, -cube->h2, 10 * gnd->height - 10 * tileHovered.y + 10);
		glm::vec3 v2(10 * tileHovered.x + 10, -cube->h4, 10 * gnd->height - 10 * tileHovered.y);
		glm::vec3 v3(10 * tileHovered.x + 10, -gnd->cubes[tileHovered.x + 1][tileHovered.y]->h1, 10 * gnd->height - 10 * tileHovered.y + 10);
		glm::vec3 v4(10 * tileHovered.x + 10, -gnd->cubes[tileHovered.x + 1][tileHovered.y]->h3, 10 * gnd->height - 10 * tileHovered.y);
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
		glColor4f(1, 1, 1, 1);
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

	if (hovered)
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			selectionStart = tileHovered;
			selectionSide = side;
		}
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			glm::ivec2 tile = selectionStart;
			for (int i = 0; i < 100; i++)
			{
				if (gnd->inMap(tile) && gnd->inMap(tile + glm::ivec2(1, 0)) && gnd->inMap(tile + glm::ivec2(0, 1)))
				{
					auto cube = gnd->cubes[tile.x][tile.y];
					glm::vec3 v1(10 * tile.x + 10, -cube->h2, 10 * gnd->height - 10 * tile.y + 10);
					glm::vec3 v2(10 * tile.x + 10, -cube->h4, 10 * gnd->height - 10 * tile.y);
					glm::vec3 v3(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h1, 10 * gnd->height - 10 * tile.y + 10);
					glm::vec3 v4(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h3, 10 * gnd->height - 10 * tile.y);
					if (side == 2)
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
					glColor4f(1, 1, 1, 1);
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
				if (side == 1 && tile.y == tileHovered.y)
					break;
				if (side == 2 && tile.x == tileHovered.x)
					break;
				if (side == 1)
					tile += glm::ivec2(0, glm::sign(tileHovered.y - selectionStart.y));
				else
					tile += glm::ivec2(glm::sign(tileHovered.x - selectionStart.x), 0);
				if (tileHovered == selectionStart)
					break;
			}
		}
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			selectedWalls.clear();
			glm::ivec2 tile = selectionStart;
			for (int i = 0; i < 100; i++)
			{
				if (gnd->inMap(tile))
					selectedWalls.push_back(glm::ivec3(tile, selectionSide));
				if (side == 1 && tile.y == tileHovered.y)
					break;
				if (side == 2 && tile.x == tileHovered.x)
					break;
				if (side == 1)
					tile += glm::ivec2(0, glm::sign(tileHovered.y - selectionStart.y));
				else
					tile += glm::ivec2(glm::sign(tileHovered.x - selectionStart.x), 0);
				if (tileHovered == selectionStart)
					break;
			}
		}
	}
	if (selectedWalls.size() > 0)
	{
		std::vector<VertexP3T2N3> verts;
		std::vector<VertexP3T2N3> topbotverts;

		WallCalculation calculation;
		calculation.prepare(browEdit);

		for (const auto& wall : selectedWalls)
		{
			glm::ivec2 tile = glm::ivec2(wall);

			if (gnd->inMap(tile) && gnd->inMap(tile + glm::ivec2(1, 0)) && gnd->inMap(tile + glm::ivec2(0, 1)))
			{
				auto cube = gnd->cubes[tile.x][tile.y];
				glm::vec3 v1(10 * tile.x + 10, -cube->h4, 10 * gnd->height - 10 * tile.y);
				glm::vec3 v2(10 * tile.x + 10, -cube->h2, 10 * gnd->height - 10 * tile.y + 10);
				glm::vec3 v3(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h3, 10 * gnd->height - 10 * tile.y);
				glm::vec3 v4(10 * tile.x + 10, -gnd->cubes[tile.x + 1][tile.y]->h1, 10 * gnd->height - 10 * tile.y + 10);
				glm::vec3 normal = glm::vec3(1, 0, 0);
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
						topbotverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, wallTop, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
						topbotverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallTop, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
					}
					if (!wallBottomAuto)
					{
						topbotverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallBottom, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
						topbotverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x, wallBottom, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
					}
				}
				else
				{
					if (!wallTopAuto)
					{
						topbotverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallTop, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
						topbotverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallTop, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0, 0), normal));
					}
					if (!wallBottomAuto)
					{
						topbotverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallBottom, 10 * gnd->height - 10 * tile.y), glm::vec2(0, 0), normal));
						topbotverts.push_back(VertexP3T2N3(glm::vec3(10 * tile.x + 10, wallBottom, 10 * gnd->height - 10 * tile.y + 10), glm::vec2(0, 0), normal));
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
			if (previewWall)
			{
				simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
				simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 1, 1));
				
				gndRenderer->textures[browEdit->activeMapView->textureSelected]->bind();
				glDrawArrays(GL_QUADS, 0, (int)verts.size());
			}
			else
			{
				simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
				simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0, 0, 1));
				glDrawArrays(GL_LINES, 0, (int)verts.size());
			}


			glEnable(GL_DEPTH_TEST);
		}

		if (topbotverts.size() > 0)
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
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), topbotverts[0].data + 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), topbotverts[0].data + 3);
			glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), topbotverts[0].data + 5);
			simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 0, 1));
			glDrawArrays(GL_LINES, 0, (int)topbotverts.size());
			glEnable(GL_DEPTH_TEST);
		}

	}



	fbo->unbind();
}