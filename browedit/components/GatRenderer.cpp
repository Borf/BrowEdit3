#include "GatRenderer.h"
#include "Gat.h"
#include <browedit/util/ResourceManager.h>
#include <browedit/Node.h>
#include <browedit/shaders/SimpleShader.h>
#include <browedit/gl/Texture.h>
#include <stb/stb_image_write.h>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <iostream>




GatRenderer::GatRenderer(gl::Texture* texture)
{
	renderContext = GatRenderContext::getInstance();
	dynamic_cast<GatRenderContext*>(renderContext)->texture = texture;
}

GatRenderer::~GatRenderer()
{
	for (auto r : chunks)
		for (auto c : r)
			delete c;
}

void GatRenderer::render()
{
	if (!this->gat)
	{
		this->gat = node->getComponent<Gat>();
		if (gat)
		{
			chunks.resize((int)ceil(gat->height / (float)CHUNKSIZE), std::vector<Chunk*>((int)ceil(gat->width / (float)CHUNKSIZE), NULL));
			for (size_t y = 0; y < chunks.size(); y++)
				for (size_t x = 0; x < chunks[y].size(); x++)
					chunks[y][x] = new Chunk((int)(x * CHUNKSIZE), (int)(y * CHUNKSIZE), gat, this);
		}
	}
	if (!this->gat)
		return;

	auto shader = dynamic_cast<GatRenderContext*>(renderContext)->shader;
	shader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::translate(glm::mat4(1.0f), glm::vec3(0, cameraDistance/1000.0f, 0)));

	for (auto r : chunks)
	{
		for (auto c : r)
		{
			if (allDirty)
				c->dirty = true;
			c->render();
		}
	}
	allDirty = false;
}


GatRenderer::GatRenderContext::GatRenderContext() : shader(util::ResourceManager<gl::Shader>::load<SimpleShader>())
{
	order = 2;
	shader->use();
	shader->setUniform(SimpleShader::Uniforms::s_texture, 0);
}


void GatRenderer::GatRenderContext::preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
{
	texture->bind();
	shader->use();
	shader->setUniform(SimpleShader::Uniforms::projectionMatrix, projectionMatrix);
	shader->setUniform(SimpleShader::Uniforms::viewMatrix, viewMatrix);
	shader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
	shader->setUniform(SimpleShader::Uniforms::lightMin, 0.5f);
	shader->setUniform(SimpleShader::Uniforms::color, glm::vec4(1, 1, 1, 1));
	glEnable(GL_BLEND);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4); //TODO: vao
}


GatRenderer::Chunk::Chunk(int x, int y, Gat* gat, GatRenderer* renderer)
{
	this->dirty = true;
	this->rebuilding = false;
	this->x = x;
	this->y = y;
	this->gat = gat;
	this->renderer = renderer;
}

GatRenderer::Chunk::~Chunk()
{
}


void GatRenderer::Chunk::render()
{
	if ((dirty || vbo.size() == 0) && !rebuilding)
		rebuild();

	if (vbo.size() > 0)
	{
		auto shader = dynamic_cast<GatRenderContext*>(renderer->renderContext)->shader;
		shader->setUniform(SimpleShader::Uniforms::colorMult, glm::vec4(1, 1, 1, renderer->opacity));
		//TODO: vao
		vbo.bind();
		//VertexP3T2N3
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), (void*)(0 * sizeof(float)));
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), (void*)(3 * sizeof(float)));
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), (void*)(5 * sizeof(float)));
		glDrawArrays(GL_TRIANGLES, 0, (int)vbo.size());
		vbo.unBind();
		shader->setUniform(SimpleShader::Uniforms::colorMult, glm::vec4(1, 1, 1, 1.0f));
	}
}


void GatRenderer::Chunk::rebuild()
{
	std::vector<VertexP3T2N3> vertices;

	for (int x = this->x; x < glm::min(this->x + CHUNKSIZE, (int)gat->cubes.size()); x++)
	{
		for (int y = this->y; y < glm::min(this->y + CHUNKSIZE, (int)gat->cubes[x].size()); y++)
		{
			Gat::Cube* cube = gat->cubes[x][y];
			int gatType = cube->gatType;

			if (gatType < 0) {
				gatType &= ~0x80000000;
			}

			glm::vec2 t1(.25f * (gatType%4), .25f * (gatType/4));
			glm::vec2 t2 = t1 + glm::vec2(.25f);

			VertexP3T2N3 v1(glm::vec3(5 * x,	-cube->h3, 5 * gat->height - 5 * y + 5),	glm::vec2(t1.x, t1.y), cube->normal);
			VertexP3T2N3 v2(glm::vec3(5 * x + 5,-cube->h4, 5 * gat->height - 5 * y + 5),	glm::vec2(t2.x, t1.y), cube->normal);
			VertexP3T2N3 v3(glm::vec3(5 * x,	-cube->h1, 5 * gat->height - 5 * y + 10),	glm::vec2(t1.x, t2.y), cube->normal);
			VertexP3T2N3 v4(glm::vec3(5 * x + 5,-cube->h2, 5 * gat->height - 5 * y + 10),	glm::vec2(t2.x, t2.y), cube->normal);

			vertices.push_back(v4); vertices.push_back(v2); vertices.push_back(v1);
			vertices.push_back(v4); vertices.push_back(v1); vertices.push_back(v3);
//			vertices.push_back(v1); vertices.push_back(v2); vertices.push_back(v3);
//			vertices.push_back(v4); vertices.push_back(v2); vertices.push_back(v3);
		}
	}

	vbo.setData(vertices, GL_STATIC_DRAW);
	dirty = false;
	rebuilding = false;
}

void GatRenderer::setChunkDirty(int x, int y)
{
	if (this->allDirty)
		return;

	if (y >= 0 && y / CHUNKSIZE < chunks.size() &&
		x >= 0 && x / CHUNKSIZE < chunks[y / CHUNKSIZE].size())
		chunks[y / CHUNKSIZE][x / CHUNKSIZE]->dirty = true;
}

void GatRenderer::setChunksDirty()
{
	allDirty = true;
}