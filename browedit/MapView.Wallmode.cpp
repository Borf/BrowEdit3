#include <windows.h>
#include <browedit/MapView.h>
#include <browedit/BrowEdit.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/gl/FBO.h>
#include <imgui.h>


void MapView::postRenderWallMode(BrowEdit* browEdit)
{
	auto gnd = map->rootNode->getComponent<Gnd>();
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

	if (gnd->inMap(tileHovered) && gnd->inMap(tileHovered + glm::ivec2(1, 0)) && gnd->inMap(tileHovered + glm::ivec2(0, 1)))
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
	ImGui::Begin("Statusbar");
	ImGui::Text("%d", side);
	ImGui::SameLine();
	ImGui::End();

	fbo->unbind();
}