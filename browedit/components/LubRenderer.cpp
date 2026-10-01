#include "LubRenderer.h"
#include <browedit/util/ResourceManager.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/NodeRenderer.h>
#include <glad/gl.h>
#include <browedit/gl/Texture.h>
#include <browedit/gl/Vertex.h>

#include <glm/gtc/matrix_transform.hpp>

LubRenderer::LubRenderer()
{
	renderContext = LubRenderContext::getInstance();
}

LubRenderer::~LubRenderer()
{
	if (texture)
		util::ResourceManager<gl::TexturePoT>::unload(texture);
	for (auto& group : animatedTextures)
		for (auto tex : group)
			util::ResourceManager<gl::TexturePoT>::unload(tex);
}

void LubRenderer::render(NodeRenderContext& context)
{
	if (!rswObject)
		rswObject = node->getComponent<RswObject>();
	if (!gnd)
		gnd = node->root->getComponent<Gnd>();
	if (!billboardRenderer)
		billboardRenderer = node->getComponent<BillboardRenderer>();
	
	auto rswEffect = node->getComponent<RswEffect>();
	bool typeChanged = false;

	if (rswEffect && rswEffect->id != effectId) {
		typeChanged = true;
		dirty = true;
	}

	if (!lubEffect || dirty || lubEffect->dirty)
	{
		lubEffect = node->getComponent<LubEffect>();
		dirty = false;

		if (texture != nullptr) {
			util::ResourceManager<gl::TexturePoT>::unload(texture);
			texture = nullptr;
		}

		for (auto& group : animatedTextures)
			for (auto tex : group)
				util::ResourceManager<gl::TexturePoT>::unload(tex);

		animatedTextures.clear();
		effectId = RswEffect::EffectType::Emitter;
		lubEffect->animatedTexture = false;

		if (lubEffect && rswEffect && rswEffect->id == RswEffect::EffectType::AnimatedEmitter) {
			animatedTextures.resize(2);
			effectId = RswEffect::EffectType::AnimatedEmitter;
			lubEffect->animatedTexture = true;

			// TODO: Should probably use GL_TEXTURE_2D_ARRAY instead of this...
			animatedTextures[0].push_back(util::ResourceManager<gl::TexturePoT>::load("data\\texture\\effect\\shockwave_b.bmp"));
			animatedTextures[0].push_back(util::ResourceManager<gl::TexturePoT>::load("data\\texture\\effect\\shockwave_c.bmp"));
			animatedTextures[0].push_back(util::ResourceManager<gl::TexturePoT>::load("data\\texture\\effect\\shockwave_d.bmp"));
			animatedTextures[0].push_back(util::ResourceManager<gl::TexturePoT>::load("data\\texture\\effect\\shockwave_e.bmp"));

			animatedTextures[1].push_back(util::ResourceManager<gl::TexturePoT>::load("data\\texture\\effect\\plazma_a.bmp"));
			animatedTextures[1].push_back(util::ResourceManager<gl::TexturePoT>::load("data\\texture\\effect\\plazma_b.bmp"));
			animatedTextures[1].push_back(util::ResourceManager<gl::TexturePoT>::load("data\\texture\\effect\\plazma_c.bmp"));
		}
		else if (lubEffect && lubEffect->texture != "")
		{
			texture = util::ResourceManager<gl::TexturePoT>::load("data\\texture\\" + util::utf8_to_iso_8859_1(util::replace(lubEffect->texture, "\\\\", "\\")));
		}
		else {
			texture = nullptr;
		}

		if (typeChanged)
			particles.clear();

		if (lubEffect)
			lubEffect->dirty = false;
	}
	if (!lubEffect)
		return;

	float currentTime = context.time;
	time += currentTime - lastTime;
	lastTime = currentTime;

	auto shader = dynamic_cast<LubRenderContext*>(renderContext)->shader;//TODO: don't cast

	glm::mat4 modelMatrix(1.0f);

	if (rswObject && gnd) {
		modelMatrix = glm::scale(modelMatrix, glm::vec3(1, 1, -1));
		modelMatrix = glm::translate(modelMatrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y + 10, -10 - 5 * gnd->height + rswObject->position.z));
	}
	else if (nextEmitTime == 0.0f) { // Nothing was created yet, this is from the lub picker
		float emitTime = 0;
		int start = (int)particles.size();

		for (int i = start; i < lubEffect->maxcount; i++) {
			emitParticle();

			particles[i].tickStart = time - emitTime;

			emitTime = emitTime + 1.0f / (lubEffect->rate.x + (rand() / (float)RAND_MAX) * (lubEffect->rate.y - lubEffect->rate.x));
		}

		if (start < lubEffect->maxcount) {
			float initialParticleAge = particles[start].duration / 2.0f;
		
			for (int i = start; i < lubEffect->maxcount; i++)
				particles[i].tickStart -= initialParticleAge;
		}
	}

	if (lubEffect->billboard_off) {
		modelMatrix = glm::rotate(modelMatrix, -glm::radians(lubEffect->rotate_angle.z), glm::vec3(0, 0, 1));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(lubEffect->rotate_angle.y), glm::vec3(0, 1, 0));
		modelMatrix = glm::rotate(modelMatrix, -glm::radians(lubEffect->rotate_angle.x), glm::vec3(1, 0, 0));
	}

	// Gravity somehow decided to move it by 1 on the Z axis, making the center rotation off.
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 0, 1));

	shader->setUniform(LubShader::Uniforms::billboard_off, (bool)lubEffect->billboard_off);
	shader->setUniform(LubShader::Uniforms::scale, lubEffect->scale);
	shader->setUniform(LubShader::Uniforms::color, glm::vec4(lubEffect->color.r, lubEffect->color.g, lubEffect->color.b, lubEffect->color.a));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (texture)
		texture->bind();
	else
		glBindTexture(GL_TEXTURE_2D, 0);
	if (texture && !texture->loaded)
		glBindTexture(GL_TEXTURE_2D, 0);

	for (int i = 0; i < particles.size(); i++) {
		auto p = &particles[i];
		float t = time - p->tickStart;
		p->position = p->startPosition + p->dir * lubEffect->speed * t + lubEffect->gravity * glm::vec3(1, -1, 1) * lubEffect->speed * t * t * 0.5f;

		if (lubEffect->eternity)
			p->alpha = lubEffect->color.w;
		else if (p->duration > 1) {
			float tRemaining = p->tickStart + p->duration - time;
			if (t < 1)
				p->alpha = t;
			else if (tRemaining < 1)
				p->alpha = tRemaining;
			else
				p->alpha = 1;
		}

		if (!lubEffect->eternity && time >= p->tickStart + p->duration)
			p->toDelete = true;
		if (i >= lubEffect->maxcount)
			p->toDelete = true;
	}

	if (particles.size() > 0)
		particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.toDelete; }), particles.end());
	
	if (time >= nextEmitTime && particles.size() < lubEffect->maxcount)
	{
		emitParticle();
		nextEmitTime = time + 1.0f / (lubEffect->rate.x + (rand() / (float)RAND_MAX) * (lubEffect->rate.y - lubEffect->rate.x));
	}

	if (lubEffect->zenable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	int src = util::d3dToOpenGlSrcBlend(lubEffect->srcmode);
	int dst = util::d3dToOpenGlDstBlend(lubEffect->destmode);
	glBlendFuncSeparate(src, dst, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(0);

	std::vector<VertexP2T2A1> verts;
	for (const auto& p : particles)
	{
		float alpha = p.alpha;
		verts.push_back(VertexP2T2A1(glm::vec2(-p.size, -p.size), glm::vec2(0, 0), alpha));
		verts.push_back(VertexP2T2A1(glm::vec2(-p.size, p.size), glm::vec2(0, 1), alpha));
		verts.push_back(VertexP2T2A1(glm::vec2(p.size, p.size), glm::vec2(1, 1), alpha));
		verts.push_back(VertexP2T2A1(glm::vec2(p.size, -p.size), glm::vec2(1, 0), alpha));
	}

	if (verts.size() > 0)
	{
		glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(VertexP2T2A1), verts[0].data);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP2T2A1), verts[0].data + 2);
		glVertexAttribPointer(2, 1, GL_FLOAT, false, sizeof(VertexP2T2A1), verts[0].data + 4);
	}

	for (int i = 0; i < particles.size(); i++)
	{
		Particle p = particles[i];
		glm::mat4 modelMatrixSub = modelMatrix;
		modelMatrixSub[3] += glm::vec4(p.position.x, p.position.y, -p.position.z, 0.0f);

		if (lubEffect->animatedTexture) {
			int texId = (int)((time - p.tickStart + p.uvDelayStart) / 0.13f) % (int)animatedTextures[p.animatedTexture].size();
			animatedTextures[p.animatedTexture][texId]->bind();
		}

		shader->setUniform(LubShader::Uniforms::modelMatrix, modelMatrixSub);
		glDrawArrays(GL_QUADS, 4 * i, 4);
	}

	glDepthMask(1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	if (ImGui::GetIO().KeyShift && billboardRenderer != nullptr && billboardRenderer->selected) {
		glLineWidth(2.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		shader->setUniform(LubShader::Uniforms::selection, true);

		for (int i = 0; i < particles.size(); i++)
		{
			Particle p = particles[i];
			glm::mat4 modelMatrixSub = modelMatrix;
			modelMatrixSub[3] += glm::vec4(p.position.x, p.position.y, -p.position.z, 0.0f);
			shader->setUniform(LubShader::Uniforms::modelMatrix, modelMatrixSub);
			glDrawArrays(GL_QUADS, 4 * i, 4);
		}

		shader->setUniform(LubShader::Uniforms::selection, false);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}

void LubRenderer::emitParticle()
{
	Particle p;
	p.position = p.startPosition = glm::vec3(
		-lubEffect->radius.x + (rand() / (float)RAND_MAX) * (-lubEffect->radius.x + 2 * abs(lubEffect->radius.x) + lubEffect->radius.x),
		-lubEffect->radius.y + (rand() / (float)RAND_MAX) * (-lubEffect->radius.y + 2 * abs(lubEffect->radius.y) + lubEffect->radius.y),
		-lubEffect->radius.z + (rand() / (float)RAND_MAX) * (-lubEffect->radius.z + 2 * abs(lubEffect->radius.z) + lubEffect->radius.z)
	);
	p.speed = glm::vec3(0);
	p.dir = glm::vec3(
		lubEffect->dir1.x + (rand() / (float)RAND_MAX) * (lubEffect->dir2.x - lubEffect->dir1.x),
		-lubEffect->dir1.y + (rand() / (float)RAND_MAX) * (-lubEffect->dir2.y + lubEffect->dir1.y),
		lubEffect->dir1.z + (rand() / (float)RAND_MAX) * (lubEffect->dir2.z - lubEffect->dir1.z)
	);
	p.duration = lubEffect->life.x + (rand() / (float)RAND_MAX) * (lubEffect->life.y - lubEffect->life.x);
	p.tickStart = time;
	p.size = lubEffect->size.x + (rand() / (float)RAND_MAX) * (lubEffect->size.y - lubEffect->size.x);
	p.alpha = 0;
	p.toDelete = false;

	if (lubEffect->animatedTexture) {
		p.animatedTexture = ((int)((rand() / (float)RAND_MAX) * 2)) % animatedTextures.size();
		p.uvDelayStart = (rand() / (float)RAND_MAX);
	}

	particles.push_back(p);
}

LubRenderer::LubRenderContext::LubRenderContext() : shader(util::ResourceManager<gl::Shader>::load<LubShader>())
{
	shader->use();
	shader->setUniform(LubShader::Uniforms::s_texture, 0);
	order = RendererDrawPriority::Lub;
}

void LubRenderer::LubRenderContext::preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers)
{
	glEnable(GL_DEPTH_TEST);
	shader->use();
	shader->setUniform(LubShader::Uniforms::projectionMatrix, context.projectionMatrix);
	shader->setUniform(LubShader::Uniforms::cameraMatrix, context.viewMatrix);
	shader->setUniform(LubShader::Uniforms::color, glm::vec4(1,1,1,1));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4); //TODO: vao
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
