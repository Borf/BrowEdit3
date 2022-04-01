#include "MapView.h"
#include <browedit/shaders/SimpleShader.h>
#include <browedit/gl/FBO.h>
#include <browedit/Node.h>
#include <browedit/Map.h>
#include <browedit/components/Gnd.h>

void MapView::postRenderTextureMode(BrowEdit* browEdit)
{
	simpleShader->use();
	simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
	simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
	simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);


	fbo->bind();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDisable(GL_TEXTURE_2D);

	if (textureGridVbo->size() > 0)
	{
		if (snapToGrid)
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
	}
	else
	{
		std::vector<VertexP3T2> verts;

		auto gnd = map->rootNode->getComponent<Gnd>();
		for (int x = 1; x < gnd->width - 1; x++)
		{
			for (int y = 1; y < gnd->height - 1; y++)
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
		}
		textureGridVbo->setData(verts, GL_STATIC_DRAW);
	}



	fbo->unbind();

}