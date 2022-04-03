#include "LubRenderer.h"
#include <browedit/util/ResourceManager.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/gl/Texture.h>
#include <browedit/gl/Vertex.h>

#include <glm/gtc/matrix_transform.hpp>

LubRenderer::LubRenderer()
{
	renderContext = LubRenderContext::getInstance();


}


int d3dToOpenGlBlend(int d3d)
{
	switch (d3d)
	{
	case 1:		return GL_ZERO;
	case 2:		return GL_ONE;
	case 3:		return GL_SRC_COLOR;
	case 4:		return GL_ONE_MINUS_SRC_COLOR;
	case 5:		return GL_SRC_ALPHA;
	case 6:		return GL_ONE_MINUS_SRC_ALPHA;
	case 7:		return GL_DST_ALPHA;
	case 8:		return GL_ONE_MINUS_DST_ALPHA;
	case 9:		return GL_DST_COLOR;
	case 10:	return GL_ONE_MINUS_DST_COLOR;
	}
	return GL_ONE;
}

void LubRenderer::render()
{
	if (!rswObject)
		rswObject = node->getComponent<RswObject>();
	if (!gnd)
		gnd = node->root->getComponent<Gnd>();
	if (!lubEffect)
	{
		lubEffect = node->getComponent<LubEffect>();
		if (lubEffect && lubEffect->texture != "")
		{
			texture = util::ResourceManager<gl::Texture>::load("data\\texture\\" + lubEffect->texture);
		}
		else
			texture = nullptr;
	}
	if (!rswObject || !lubEffect)
		return;

	auto shader = dynamic_cast<LubRenderContext*>(renderContext)->shader;//TODO: don't cast

	glm::mat4 modelMatrix(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1, 1, -1));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));
	shader->setUniform(LubShader::Uniforms::modelMatrix, modelMatrix);
	shader->setUniform(LubShader::Uniforms::color, glm::vec4(lubEffect->color, 255.0f) / glm::vec4(255.0f));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (texture)
		texture->bind();
	else
		glBindTexture(GL_TEXTURE_2D, 0);

	float time = (float)glfwGetTime();
	float elapsedTime = time - lastTime;
	lastTime = time;
	for (auto& p : particles)
	{
		p.position += p.speed * elapsedTime;
		p.life -= elapsedTime;
		p.speed += lubEffect->gravity * elapsedTime;
	}
	if(particles.size() > 0)
		particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life < 0; }), particles.end());
	emitTime -= elapsedTime;
	if (emitTime < 0 && particles.size() < lubEffect->maxcount)
	{
		Particle p;
		p.position = glm::vec3(0.0f);
		p.speed.x = lubEffect->dir1.x + (rand() / (float)RAND_MAX) * (lubEffect->dir2.x - lubEffect->dir1.x);
		p.speed.y = -(lubEffect->dir1.y + (rand() / (float)RAND_MAX) * (lubEffect->dir2.y - lubEffect->dir1.z));
		p.speed.z = lubEffect->dir1.z + (rand() / (float)RAND_MAX) * (lubEffect->dir2.z - lubEffect->dir1.y);
		p.life = lubEffect->life.x + (rand() / (float)RAND_MAX) * (lubEffect->life.y - lubEffect->life.x);
		p.size = lubEffect->size.x + (rand() / (float)RAND_MAX) * (lubEffect->size.y - lubEffect->size.x);

		particles.push_back(p);
		emitTime = 1.0f / (lubEffect->rate.x + (rand() / (float)RAND_MAX) * (lubEffect->rate.y - lubEffect->rate.x));
	}


	glEnable(GL_BLEND);
	int src = d3dToOpenGlBlend(lubEffect->srcmode);
	int dst = d3dToOpenGlBlend(lubEffect->srcmode);
	glBlendFuncSeparate(src, dst, GL_ONE, GL_ONE);
	glDepthMask(0);

	std::vector<VertexP3T2A1> verts;
	for (const auto& p : particles)
	{
		float alpha = glm::clamp(p.life, 0.0f, 1.0f);
		verts.push_back(VertexP3T2A1(p.position + glm::vec3(-p.size/2, 0, 0), glm::vec2(0, 0), alpha));
		verts.push_back(VertexP3T2A1(p.position + glm::vec3(-p.size/2, p.size, 0), glm::vec2(0, 1), alpha));
		verts.push_back(VertexP3T2A1(p.position + glm::vec3(p.size/2, p.size, 0), glm::vec2(1, 1), alpha));
		verts.push_back(VertexP3T2A1(p.position + glm::vec3(p.size/2, 0, 0), glm::vec2(1, 0), alpha));
	}


	if (verts.size() > 0)
	{

		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2A1), verts[0].data);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2A1), verts[0].data + 3);
		glVertexAttribPointer(2, 1, GL_FLOAT, false, sizeof(VertexP3T2A1), verts[0].data + 5);
		glDrawArrays(GL_QUADS, 0, (int)verts.size());
	}
	glDepthMask(1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}


LubRenderer::LubRenderContext::LubRenderContext() : shader(util::ResourceManager<gl::Shader>::load<LubShader>())
{
	shader->use();
	shader->setUniform(LubShader::Uniforms::s_texture, 0);
	order = 4;
}

void LubRenderer::LubRenderContext::preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
{
	glEnable(GL_DEPTH_TEST);
	shader->use();
	shader->setUniform(LubShader::Uniforms::projectionMatrix, projectionMatrix);
	shader->setUniform(LubShader::Uniforms::cameraMatrix, viewMatrix);
	shader->setUniform(LubShader::Uniforms::color, glm::vec4(1,1,1,1));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4); //TODO: vao
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
