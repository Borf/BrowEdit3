#include "WaterRenderer.h"
#include "Rsw.h"
#include "Gnd.h"
#include <browedit/util/ResourceManager.h>
#include <browedit/Node.h>
#include <browedit/shaders/WaterShader.h>
#include <browedit/gl/Texture.h>
#include <stb/stb_image_write.h>
#include <map>
#include <iostream>




WaterRenderer::WaterRenderer()
{
	renderContext = WaterRenderContext::getInstance();
}

WaterRenderer::~WaterRenderer()
{
	for (auto t : textures)
		util::ResourceManager<gl::Texture>::unload(t);
}

void WaterRenderer::setDirty()
{
	dirty = true;
}

void WaterRenderer::render()
{
	if (!this->rsw || dirty)
	{ //todo: move a tryLoadGnd method
		this->gnd = node->getComponent<Gnd>();
		this->rsw = node->getComponent<Rsw>();
		
		if (!rsw) {
			dirty = false;
			return;
		}

		reloadTextures();

		if (!vbo || dirty)
		{
			float waveHeight = -9999;

			int perWidth = gnd->width / glm::max(1, rsw->water.splitWidth);
			int perHeight = gnd->height / glm::max(1, rsw->water.splitHeight);
			int prevOffset = 0;

			if (!vbo)
				vbo = new gl::VBO<VertexP3T2>();
			
			vertIndices.clear();
			std::vector<VertexP3T2> verts;

			for (int yy = rsw->water.splitHeight - 1; yy >= 0; yy--) {
				for (int xx = 0; xx < rsw->water.splitWidth; xx++) {
					auto water = &rsw->water.zones[xx][rsw->water.splitHeight - yy - 1];

					waveHeight = water->height - water->amplitude;

					int xmax = perWidth * (xx + 1);

					if (xx == rsw->water.splitWidth - 1)
						xmax = gnd->width;

					int ymax = perHeight * (yy + 1);

					if (ymax == rsw->water.splitHeight - 1)
						ymax = gnd->height;

					int vertsCount = 0;

					for (int x = perWidth * xx; x < xmax; x++) {
						for (int y = perHeight * yy; y < ymax; y++) {
							auto c = gnd->cubes[x][gnd->height - y - 1];

							if (!this->renderFullWater) {
								if (c->tileUp == -1)
									continue;

								if (c->heights[0] <= waveHeight && c->heights[1] <= waveHeight && c->heights[2] <= waveHeight && c->heights[3] <= waveHeight)
									continue;
							}
					
							verts.push_back(VertexP3T2(glm::vec3(10 * x,		0, 10 * (y+1)),	glm::vec2((x % 4) * 0.25f + 0.00f, (y % 4) * 0.25f + 0.00f)));
							verts.push_back(VertexP3T2(glm::vec3(10 * (x+1),	0, 10 * (y+1)),	glm::vec2((x % 4) * 0.25f + 0.25f, (y % 4) * 0.25f + 0.00f)));
							verts.push_back(VertexP3T2(glm::vec3(10 * (x+1),	0, 10 * (y+2)), glm::vec2((x % 4) * 0.25f + 0.25f, (y % 4) * 0.25f + 0.25f)));
							verts.push_back(VertexP3T2(glm::vec3(10 * x,		0, 10 * (y+2)), glm::vec2((x % 4) * 0.25f + 0.00f, (y % 4) * 0.25f + 0.25f)));
							vertsCount += 4;
						}
					}
					
					vertIndices.push_back(VboIndex(32 * (int)vertIndices.size(), prevOffset, vertsCount));
					prevOffset += vertsCount;
				}
			}

			vbo->setData(verts, GL_STATIC_DRAW);
			dirty = false;
		}
	}
	if (!this->rsw)
		return;

	if (textures.size() == 0)
		return;

	float time = (float)glfwGetTime();

	auto shader = dynamic_cast<WaterRenderContext*>(renderContext)->shader;
	shader->setUniform(WaterShader::Uniforms::time, time);
	shader->setUniform(WaterShader::Uniforms::fogEnabled, viewFog);
	shader->setUniform(WaterShader::Uniforms::fogNear, rsw->fog.nearPlane * 240 * 2.5f);
	shader->setUniform(WaterShader::Uniforms::fogFar, rsw->fog.farPlane * 240 * 2.5f);
	//shader->setUniform(WaterShader::Uniforms::fogExp, rsw->fog.factor);
	shader->setUniform(WaterShader::Uniforms::fogColor, rsw->fog.color);

	glDepthMask(0);
	glEnable(GL_BLEND);
	vbo->bind();

	for (int y = 0; y < rsw->water.splitHeight; y++) {
		for (int x = 0; x < rsw->water.splitWidth; x++) {
			auto water = &rsw->water.zones[x][y];

			shader->setUniform(WaterShader::Uniforms::waterHeight, -water->height);

			if (gnd && gnd->version <= 0x108) {
				water = &rsw->water.zones[0][0];
				shader->setUniform(WaterShader::Uniforms::amplitude, water->amplitude);
				shader->setUniform(WaterShader::Uniforms::waveSpeed, water->waveSpeed);
				shader->setUniform(WaterShader::Uniforms::wavePitch, water->wavePitch);
			}
			else {
				shader->setUniform(WaterShader::Uniforms::waterHeight, -water->height);
				shader->setUniform(WaterShader::Uniforms::amplitude, water->amplitude);
				shader->setUniform(WaterShader::Uniforms::waveSpeed, water->waveSpeed);
				shader->setUniform(WaterShader::Uniforms::wavePitch, water->wavePitch);
			}

			int vertIndex = y * rsw->water.splitWidth + x;
			int index = ((int)(time * 60 / water->textureAnimSpeed)) % 32;
			textures[(gnd && gnd->version <= 0x108 ? 0 : vertIndices[vertIndex].texture) + index]->bind();
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(0 * sizeof(float)));
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(3 * sizeof(float)));
			glDrawArrays(GL_QUADS, (int)vertIndices[vertIndex].begin, (int)vertIndices[vertIndex].count);
		}
	}

	glDepthMask(1);
}


void WaterRenderer::reloadTextures()
{
	// Only reload necessary textures, it prevents lag issues
	for (int y = 0; y < rsw->water.splitHeight; y++) {
		for (int x = 0; x < rsw->water.splitWidth; x++) {
			for (int i = 0; i < 32; i++) {
				char buf[128];
				sprintf_s(buf, 128, "data/texture/ฟ๖ลอ/water%i%02i%s", rsw->water.zones[x][y].type, i, ".jpg");

				int index = 32 * (y * rsw->water.splitWidth + x) + i;
				if (index >= textures.size()) {
					textures.push_back(util::ResourceManager<gl::Texture>::load(buf));
				}
				else {
					if (textures[index]->fileName != buf) {
						util::ResourceManager<gl::Texture>::unload(textures[index]);
						textures[index] = util::ResourceManager<gl::Texture>::load(buf);
					}
				}
			}
		}
	}
}


WaterRenderer::WaterRenderContext::WaterRenderContext() : shader(util::ResourceManager<gl::Shader>::load<WaterShader>())
{
	order = 2;
	shader->use();
	shader->setUniform(WaterShader::Uniforms::s_texture, 0);
}


void WaterRenderer::WaterRenderContext::preFrame(Node* rootNode, const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
{
	shader->use();
	shader->setUniform(WaterShader::Uniforms::ProjectionMatrix, projectionMatrix);
	shader->setUniform(WaterShader::Uniforms::ViewMatrix, viewMatrix);
	shader->setUniform(WaterShader::Uniforms::ModelMatrix, glm::mat4(1.0f));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4); //TODO: vao
}

