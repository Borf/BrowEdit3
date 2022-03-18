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

void WaterRenderer::render()
{
	if (!this->rsw)
	{ //todo: move a tryLoadGnd method
		this->rsw = node->getComponent<Rsw>();
		if (rsw)
		{
			reloadTextures();
		}

		if (!vbo)
		{
			auto gnd = node->getComponent<Gnd>();
			vbo = new gl::VBO<VertexP3T2>();
			std::vector<VertexP3T2> verts;
			for (int x = 0; x < gnd->width; x++)
			{
				for (int y = 0; y < gnd->height; y++)
				{
					verts.push_back(VertexP3T2(glm::vec3(10 * x,		0, 10 * y), glm::vec2(0, 0)));
					verts.push_back(VertexP3T2(glm::vec3(10 * (x+1),	0, 10 * y), glm::vec2(1, 0)));
					verts.push_back(VertexP3T2(glm::vec3(10 * (x+1),	0, 10 * (y+1)), glm::vec2(1, 1)));
					verts.push_back(VertexP3T2(glm::vec3(10 * x,		0, 10 * (y+1)), glm::vec2(0, 1)));
				}
			}
			vbo->setData(verts, GL_STATIC_DRAW);
		}
	}
	if (!this->rsw)
		return;

	auto shader = dynamic_cast<WaterRenderContext*>(renderContext)->shader;
	shader->setUniform(WaterShader::Uniforms::time, (float)glfwGetTime());
	shader->setUniform(WaterShader::Uniforms::waterHeight, -rsw->water.height);
	shader->setUniform(WaterShader::Uniforms::amplitude, rsw->water.amplitude);
	shader->setUniform(WaterShader::Uniforms::animSpeed, (float)rsw->water.animSpeed);

	glEnable(GL_BLEND);

	vbo->bind();
	textures[((int)(glfwGetTime()*rsw->water.animSpeed)) % textures.size()]->bind();
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(0 * sizeof(float)));
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(3 * sizeof(float)));
	glDrawArrays(GL_QUADS, 0, (int)vbo->size());



	
}


void WaterRenderer::reloadTextures()
{
	for (auto t : textures)
		util::ResourceManager<gl::Texture>::unload(t);
	textures.clear();
	for (int i = 0; i < 32; i++)
	{
		char buf[128];
		sprintf_s(buf, 128, "data/texture/ฟ๖ลอ/water%i%02i%s", rsw->water.type, i, ".jpg");
		textures.push_back(util::ResourceManager<gl::Texture>::load(buf));
		//textures.back()->setTextureRepeat(true);
	}
}


WaterRenderer::WaterRenderContext::WaterRenderContext() : shader(util::ResourceManager<gl::Shader>::load<WaterShader>())
{
	order = 2;
	shader->use();
	shader->setUniform(WaterShader::Uniforms::s_texture, 0);
}


void WaterRenderer::WaterRenderContext::preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
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

