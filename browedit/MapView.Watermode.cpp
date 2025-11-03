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
#include <browedit/components/WaterRenderer.h>
#include <browedit/components/Rsw.h>
#include <browedit/math/Polygon.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/TileSelectAction.h>
#include <browedit/actions/CubeHeightChangeAction.h>
#include <browedit/actions/CubeTileChangeAction.h>
#include <browedit/actions/NewObjectAction.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/WaterHeightChangeAction.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <queue>

void MapView::rebuildWaterGrid(Rsw* rsw, Gnd* gnd, bool forced)
{
	std::vector<VertexP3> verts;
	glm::vec3 n(0, -1, 0);

	if (waterGridDirty || forced || rsw->water.splitHeight * rsw->water.splitWidth * 8 != waterGridVbo->size()) {
		for (int x = 0; x < rsw->water.splitWidth; x++)
		{
			for (int y = 0; y < rsw->water.splitHeight; y++)
			{
				int xmin, xmax, ymin, ymax;
				auto zone = &rsw->water.zones[x][y];

				rsw->water.getBoundsFromGnd(x, y, gnd, &xmin, &xmax, &ymin, &ymax);

				VertexP3 v0 = VertexP3(glm::vec3(10 * xmin, -zone->height, 10 * gnd->height - 10 * ymin));
				VertexP3 v1 = VertexP3(glm::vec3(10 * xmax, -zone->height, 10 * gnd->height - 10 * ymax));
				VertexP3 v2 = VertexP3(glm::vec3(10 * xmax, -zone->height, 10 * gnd->height - 10 * ymin));
				VertexP3 v3 = VertexP3(glm::vec3(10 * xmin, -zone->height, 10 * gnd->height - 10 * ymax));

				verts.push_back(v0); verts.push_back(v2);
				verts.push_back(v0); verts.push_back(v3);
				verts.push_back(v1); verts.push_back(v3);
				verts.push_back(v1); verts.push_back(v2);
			}
		}

		waterGridVbo->setData(verts, GL_STATIC_DRAW);
		waterGridDirty = false;
	}
}

void MapView::postRenderWaterMode(BrowEdit* browEdit)
{
	float gridSize = gridSizeTranslate;
	float gridOffset = gridOffsetTranslate;
	static bool isDragged = false;

	auto rsw = map->rootNode->getComponent<Rsw>();
	auto gnd = map->rootNode->getComponent<Gnd>();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	auto waterRenderer = map->rootNode->getComponent<WaterRenderer>();

	if (!rsw || !gnd || !gndRenderer || !waterRenderer)
		return;

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
	glm::vec3 n(0, -1, 0);

	auto mouse3D = rsw->rayCastWater(mouseRay, gnd, viewEmptyTiles);
	glm::ivec2 tileHovered;

	if (mouse3D == glm::vec3(std::numeric_limits<float>::max())) {
		tileHovered = glm::ivec2(-1, -1);
	}
	else {
		rsw->water.getIndexFromGnd((int)glm::floor(mouse3D.x / 10), gnd->height - (int)glm::floor(mouse3D.z) / 10, gnd, &tileHovered.x, &tileHovered.y);
	}

	ImGui::Begin("Statusbar");
	ImGui::SetNextItemWidth(100.0f);
	ImGui::Text("Water zone: %d,%d", tileHovered.x, tileHovered.y);
	ImGui::SameLine();
	ImGui::End();

	bool snap = snapToGrid;
	if (ImGui::GetIO().KeyShift)
		snap = !snap;

	int perWidth = glm::max(1, gnd->width / glm::max(1, rsw->water.splitWidth));
	int perHeight = glm::max(1, gnd->height / glm::max(1, rsw->water.splitHeight));

	std::vector<glm::ivec2> waterSelection;

	// Always select the entire water if there are no split zones
	if (gnd->version < 0x108 && map->waterSelection.size() == 0)
		map->waterSelection.push_back(glm::ivec2(0, 0));

	for (auto& tile : map->waterSelection)
	{
		// The selection can become invalid if the split zones count was changed
		if (tile.x < 0 || tile.x >= rsw->water.splitWidth ||
			tile.y < 0 || tile.y >= rsw->water.splitHeight)
			continue;

		waterSelection.push_back(tile);
	}

	// Draw the white water grid, it's hard to see the zones otherwise
	rebuildWaterGrid(rsw, gnd);
	
	if (showWaterGrid && !isDragged && waterGridVbo->size() > 0) {
		waterGridVbo->bind();
		glEnableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		glDisableVertexAttribArray(3);
		glDisableVertexAttribArray(4);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 1, 0.25f));
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3), (void*)(0 * sizeof(float)));
		glDrawArrays(GL_LINES, 0, (int)waterGridVbo->size());
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		waterGridVbo->unBind();
	}

	if (waterSelection.size() > 0 && canSelect)
	{
		std::vector<VertexP3> verts;
		std::vector<VertexP3> vertsGrid;

		for (auto& tile : waterSelection)
		{
			int xmin, xmax, ymin, ymax;
			auto zone = &rsw->water.zones[tile.x][tile.y];

			rsw->water.getBoundsFromGnd(tile.x, tile.y, gnd, &xmin, &xmax, &ymin, &ymax);

			VertexP3 v0 = VertexP3(glm::vec3(10 * xmin, -zone->height, 10 * gnd->height - 10 * ymin));
			VertexP3 v1 = VertexP3(glm::vec3(10 * xmax, -zone->height, 10 * gnd->height - 10 * ymax));
			VertexP3 v2 = VertexP3(glm::vec3(10 * xmax, -zone->height, 10 * gnd->height - 10 * ymin));
			VertexP3 v3 = VertexP3(glm::vec3(10 * xmin, -zone->height, 10 * gnd->height - 10 * ymax));

			verts.push_back(v0);
			verts.push_back(v1);
			verts.push_back(v2);

			verts.push_back(v3);
			verts.push_back(v0);
			verts.push_back(v1);

			vertsGrid.push_back(v0); vertsGrid.push_back(v2);
			vertsGrid.push_back(v0); vertsGrid.push_back(v3);
			vertsGrid.push_back(v1); vertsGrid.push_back(v3);
			vertsGrid.push_back(v1); vertsGrid.push_back(v2);
		}

		if (vertsGrid.size() > 0) {
			glEnableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDisableVertexAttribArray(3);
			glDisableVertexAttribArray(4);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3), verts[0].data);

			glLineWidth(1.0f);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0, 0, (gnd->version >= 0x108 && showWaterSelectedOverlay) ? 0.15f : 0.0f));
			glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0, 0, 1.0f));
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3), vertsGrid[0].data);
			glDrawArrays(GL_LINES, 0, (int)vertsGrid.size());
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		Gadget::setMatrices(nodeRenderContext.projectionMatrix, nodeRenderContext.viewMatrix);
		glDepthMask(1);

		glm::ivec2 maxValues(-99999, -99999), minValues(999999, 9999999);

		for (auto& tile : waterSelection)
		{
			maxValues = glm::max(maxValues, tile);
			minValues = glm::min(minValues, tile);
		}

		static std::map<Rsw::Water*, float> originalValues;
		static glm::vec3 clickedPos;

		int xmin = perWidth * minValues.x;
		int xmax = perWidth * (maxValues.x + 1);
		
		if (maxValues.x == rsw->water.splitWidth - 1)
			xmax = gnd->width;
		
		int ymin = perHeight * minValues.y - 1;
		int ymax = perHeight * (maxValues.y + 1) - 1;
		
		if (maxValues.y == rsw->water.splitHeight - 1)
			ymax = gnd->height - 1;

		glm::vec3 pos[5];
		glm::vec3 posGadget[5];
		posGadget[0] = pos[0] = glm::vec3(10 * xmin, -rsw->water.zones[minValues.x][minValues.y].height, 10 * gnd->height - 10 * ymin);
		posGadget[1] = pos[1] = glm::vec3(10 * xmax, -rsw->water.zones[maxValues.x][minValues.y].height, 10 * gnd->height - 10 * ymin);
		posGadget[2] = pos[2] = glm::vec3(10 * xmin, -rsw->water.zones[minValues.x][maxValues.y].height, 10 * gnd->height - 10 * ymax);
		posGadget[3] = pos[3] = glm::vec3(10 * xmax, -rsw->water.zones[maxValues.x][maxValues.y].height, 10 * gnd->height - 10 * ymax);

		glm::vec3 center = (pos[0] + pos[1] + pos[2] + pos[3]) / 4.0f;

		int cx = (maxValues.x - minValues.x) / 2 + minValues.x;
		int cy = (maxValues.y - minValues.y) / 2 + minValues.y;
		center.y = -rsw->water.zones[cx][cy].height;

		posGadget[4] = pos[4] = center;

		glm::vec3 cameraPos = glm::inverse(nodeRenderContext.viewMatrix) * glm::vec4(0, 0, 0, 1);
		int order[5] = { 0, 1, 2, 3, 4 };
		std::sort(std::begin(order), std::end(order), [&](int a, int b)
		{
			return glm::distance(cameraPos, pos[a]) < glm::distance(cameraPos, pos[b]);
		});

		static int dragIndex = -1;
		for (int ii = 0; ii < 5; ii++)
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

			if (dragIndex == -1)
				gadgetHeight[i].disabled = false;
			else
				gadgetHeight[i].disabled = dragIndex != i;

			gadgetHeight[i].draw(mouseRay, mat);

			if (gadgetHeight[i].axisClicked)
			{
				dragIndex = i;
				mouseDragPlane.normal = -mouseRay.dir;
				mouseDragPlane.normal.y = 0;
				mouseDragPlane.normal = glm::normalize(mouseDragPlane.normal);
				mouseDragPlane.D = -glm::dot(pos[i], mouseDragPlane.normal);

				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				mouseDragStart = mouseRay.origin + f * mouseRay.dir;

				originalValues.clear();
				for (auto& t : waterSelection)
					originalValues[&rsw->water.zones[t.x][t.y]] = rsw->water.zones[t.x][t.y].height;

				clickedPos = pos[i];

				canSelect = false;
				waterRenderer->renderFullWater = true;
				waterRenderer->setDirty();
			}
			else if (gadgetHeight[i].axisReleased)
			{
				dragIndex = -1;
				canSelect = false;
				isDragged = false;

				std::map<Rsw::Water*, float> newValues;
				for (auto& t : originalValues)
					newValues[t.first] = t.first->height;
				map->doAction(new WaterHeightChangeAction(originalValues, newValues, waterSelection), browEdit);
				waterRenderer->renderFullWater = false;
				waterRenderer->setDirty();

				rebuildWaterGrid(rsw, gnd, true);
			}
			else if (gadgetHeight[i].axisDragged)
			{
				isDragged = true;

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
					glDisableVertexAttribArray(1);
					glDisableVertexAttribArray(2);
					glDisableVertexAttribArray(3);
					glDisableVertexAttribArray(4);
					glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3), (void*)(0 * sizeof(float)));
					glDrawArrays(GL_LINES, 0, (int)gridVbo->size());
					gridVbo->unBind();
				}
				
				float f;
				mouseRay.planeIntersection(mouseDragPlane, f);
				glm::vec3 mouseOffset = (mouseRay.origin + f * mouseRay.dir) - mouseDragStart;

				if (snap && gridLocal)
					mouseOffset = glm::round(mouseOffset / (float)gridSize) * (float)gridSize;

				canSelect = false;

				float offsetY = clickedPos.y - mouseOffset.y;
				if (snap && !gridLocal)
				{
					offsetY += 2 * mouseOffset.y;
					offsetY = glm::round(offsetY / gridSize) * gridSize;
					offsetY = clickedPos.y + (clickedPos.y - offsetY);
				}

				for (auto& t : waterSelection)
				{
					rsw->water.zones[t.x][t.y].height = originalValues[&rsw->water.zones[t.x][t.y]] + offsetY - clickedPos.y;
				}
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

		if (ImGui::IsMouseReleased(0) && hovered)
		{
			auto mouseDragEnd = mouse3D;
			mouseDown = false;

			std::map<int, glm::ivec2> newSelection;
			if (ImGui::GetIO().KeyShift || ImGui::GetIO().KeyCtrl) {
				for (auto& tile : waterSelection) {
					newSelection[tile.x + rsw->water.splitWidth * tile.y] = tile;
				}
			}
			if (browEdit->selectTool == BrowEdit::SelectTool::Rectangle)
			{
				int waterMinX, waterMinY, waterMaxX, waterMaxY;

				rsw->water.getIndexFromGnd(
					(int)glm::floor(glm::min(mouseDragStart.x, mouseDragEnd.x) / 10),
					gnd->height - (int)glm::floor(glm::max(mouseDragStart.z, mouseDragEnd.z) / 10),
					gnd,
					&waterMinX,
					&waterMinY);

				rsw->water.getIndexFromGnd(
					(int)glm::floor(glm::max(mouseDragStart.x, mouseDragEnd.x) / 10),
					gnd->height - (int)glm::floor(glm::min(mouseDragStart.z, mouseDragEnd.z) / 10),
					gnd,
					&waterMaxX,
					&waterMaxY);

				if (waterMinX >= 0 && waterMaxX < rsw->water.splitWidth && waterMinY >= 0 && waterMaxY < rsw->water.splitHeight)
					for (int x = waterMinX; x < waterMaxX + 1; x++)
					{
						for (int y = waterMinY; y < waterMaxY + 1; y++)
						{
							if (ImGui::GetIO().KeyCtrl)
							{
								if (newSelection.contains(x + rsw->water.splitWidth * y))
									newSelection.erase(x + rsw->water.splitWidth * y);
							}
							else if (!newSelection.contains(x + rsw->water.splitWidth * y))
								newSelection[x + rsw->water.splitWidth * y] = glm::ivec2(x, y);
						}
					}
			}

			std::vector<glm::ivec2> newSelection2;

			for (const auto& entry : newSelection) {
				newSelection2.push_back(entry.second);
			}

			if (waterSelection != newSelection2)
			{
				map->doAction(new WaterZoneSelectAction(map, newSelection2), browEdit);
			}
		}
	}

	if (mouseDown)
	{
		auto mouseDragEnd = mouse3D;
		std::vector<VertexP3> verts;
		float dist = 0.002f * cameraDistance;
		int waterMinX, waterMinY, waterMaxX, waterMaxY;

		rsw->water.getIndexFromGnd(
			(int)glm::floor(glm::min(mouseDragStart.x, mouseDragEnd.x) / 10),
			gnd->height - (int)glm::floor(glm::max(mouseDragStart.z, mouseDragEnd.z) / 10),
			gnd,
			&waterMinX,
			&waterMinY);

		rsw->water.getIndexFromGnd(
			(int)glm::floor(glm::max(mouseDragStart.x, mouseDragEnd.x) / 10),
			gnd->height - (int)glm::floor(glm::min(mouseDragStart.z, mouseDragEnd.z) / 10),
			gnd,
			&waterMaxX,
			&waterMaxY);

		ImGui::Begin("Statusbar");
		ImGui::Text("Selection: (%d,%d) - (%d,%d)", waterMinX, waterMinY, waterMaxX, waterMaxY);
		ImGui::End();

		//if (browEdit->selectTool == BrowEdit::SelectTool::Rectangle)
		//{
		if (waterMinX >= 0 && waterMaxX < rsw->water.splitWidth && waterMinY >= 0 && waterMaxY < rsw->water.splitHeight)
			for (int x = waterMinX; x < waterMaxX + 1; x++)
			{
				for (int y = waterMinY; y < waterMaxY + 1; y++)
				{
					int xmin, xmax, ymin, ymax;
					auto zone = &rsw->water.zones[x][y];

					rsw->water.getBoundsFromGnd(x, y, gnd, &xmin, &xmax, &ymin, &ymax);

					verts.push_back(VertexP3(glm::vec3(10 * xmin, -zone->height + dist, 10 * gnd->height - 10 * ymin)));
					verts.push_back(VertexP3(glm::vec3(10 * xmax, -zone->height + dist, 10 * gnd->height - 10 * ymax)));
					verts.push_back(VertexP3(glm::vec3(10 * xmax, -zone->height + dist, 10 * gnd->height - 10 * ymin)));

					verts.push_back(VertexP3(glm::vec3(10 * xmin, -zone->height + dist, 10 * gnd->height - 10 * ymax)));
					verts.push_back(VertexP3(glm::vec3(10 * xmin, -zone->height + dist, 10 * gnd->height - 10 * ymin)));
					verts.push_back(VertexP3(glm::vec3(10 * xmax, -zone->height + dist, 10 * gnd->height - 10 * ymax)));
				}
			}

		if (verts.size() > 0)
		{
			glEnableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);
			glDisableVertexAttribArray(3);
			glDisableVertexAttribArray(4);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3), verts[0].data);
			simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 0, 0, 0.5f));
			glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
		}
		//}
	}

	glDepthMask(1);
	fbo->unbind();
}