#include "LubWindRenderer.h"
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
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
#include <stb/stb_image.h>

LubWindRenderer::LubWindRenderer()
{
	renderContext = LubWindRenderContext::getInstance();
}

LubWindRenderer::~LubWindRenderer()
{
	if (texture)
		util::ResourceManager<gl::TexturePoT>::unload(texture);
	if (renderInfo) {
		if (renderInfo->vbo)
			delete renderInfo->vbo;
		if (renderInfo->ebo)
			delete renderInfo->ebo;
		if (renderInfo->vao)
			delete renderInfo->vao;

		delete renderInfo;
	}
		
	if (ubo)
		delete ubo;
}

void LubWindRenderer::render(NodeRenderContext& context)
{
	if (!this->lubWindEffect)
		this->lubWindEffect = node->getComponent<LubWindEffect>();
	if (!this->gnd)
		this->gnd = node->root->getComponent<Gnd>();
	if (!this->rswObject)
		this->rswObject = node->getComponent<RswObject>();

	if (!lubWindEffect || !gnd || !rswObject || !enabled)
		return;

	// Ensure atlas are created
	if (dirty || lubWindEffect->dirty) {
		if (texture)
			util::ResourceManager<gl::TexturePoT>::unload(texture);

		texture = util::ResourceManager<gl::TexturePoT>::load("data\\texture\\" + util::utf8_to_iso_8859_1(util::replace(lubWindEffect->texture, "\\\\", "\\")));
		lubWindEffect->dirty = false;
		dirty = false;
	}

	if (!ubo)
		ubo = new gl::UBO<ParticleParams>();

	auto shader = dynamic_cast<LubWindRenderContext*>(renderContext)->shader;

	glm::mat4 modelMatrix(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1, 1, -1));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));

	time = context.time;
	shader->setUniform(LubWindShader::Uniforms::uTime, time);

	params.particleNum = (float)lubWindEffect->particleNum;
	params.radius = lubWindEffect->radius;
	params.thickness = lubWindEffect->thickness;
	params.height = lubWindEffect->height;
	params.speed = lubWindEffect->speed;
	params.fullAngle = lubWindEffect->fullAngle;
	params.rotateVector = lubWindEffect->rotateVector;
	params.basePosition = glm::vec3(modelMatrix[3]);

	if (params.particleNum <= 1)
		return;

	auto ri = renderInfo;

	// Load render data
	if (ri == nullptr) {
		renderInfo = new RenderInfo();
		ri = renderInfo;

		const int segments = 20;

		std::vector<VertexP3T2> verts;
		std::vector<unsigned int> indices;

		verts.reserve((segments + 1) * 2);
		indices.reserve(segments * 6);

		for (int i = 0; i <= segments; i++) {
			float t = (float)i / segments;

			verts.push_back(VertexP3T2(glm::vec3(t, 0.0f, 0.0f), glm::vec2(1.0f - t, 1.0f)));
			verts.push_back(VertexP3T2(glm::vec3(t, -1.0f, 0.0f), glm::vec2(1.0f - t, 0.0f)));
		}

		for (int i = 0; i < segments; i++) {
			unsigned int bottomLeft = i * 2;
			unsigned int topLeft = i * 2 + 1;
			unsigned int bottomRight = (i + 1) * 2;
			unsigned int topRight = (i + 1) * 2 + 1;

			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
			indices.push_back(topLeft);

			indices.push_back(topLeft);
			indices.push_back(bottomRight);
			indices.push_back(topRight);
		}

		ri->vao = new gl::VAO();
		ri->vao->bind();

		ri->vbo = new gl::VBO<VertexP3T2>();
		ri->vbo->setData(verts, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(0 * sizeof(float)));
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), (void*)(3 * sizeof(float)));

		ri->ebo = new gl::EBO();
		ri->ebo->setData(indices, GL_STATIC_DRAW);

		ri->instance_vbo = new gl::VBO<Particle>();
		ri->instance_vbo->setData(particles, GL_STREAM_DRAW);

		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(2, 1, GL_FLOAT, false, sizeof(Particle), (void*)(0 * sizeof(float)));
		glVertexAttribPointer(3, 1, GL_FLOAT, false, sizeof(Particle), (void*)(1 * sizeof(float)));
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
	}

	glBlendFuncSeparate(util::d3dToOpenGlSrcBlend(lubWindEffect->srcmode), util::d3dToOpenGlDstBlend(lubWindEffect->destmode), GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
	shader->setUniform(LubWindShader::Uniforms::color, lubWindEffect->color);
		
	ubo->setData(&params);
		
	texture->bind();
		
	ri->vao->bind();
	ri->instance_vbo->bind();
	
	if (particles.size() != lubWindEffect->particleNum) {
		particles.resize(lubWindEffect->particleNum);
		ri->instance_vbo->setData(particles, GL_STREAM_DRAW);

		for (int i = 0; i < particles.size(); i++) {
			auto& particle = particles[i];
			particle.seed = (rand() / (float)RAND_MAX);
		}

		ri->instance_vbo->setData(particles, GL_STREAM_DRAW);
	}
		
	ri->ebo->bind();
	glDrawElementsInstanced(GL_TRIANGLES, (int)ri->ebo->size(), GL_UNSIGNED_INT, 0, (int)particles.size());
}

LubWindRenderer::LubWindRenderContext::LubWindRenderContext() : shader(util::ResourceManager<gl::Shader>::load<LubWindShader>())
{
	shader->use();
	shader->setUniform(LubWindShader::Uniforms::s_texture, 0);
	order = RendererDrawPriority::LubWind;
}

void LubWindRenderer::LubWindRenderContext::preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthMask(false);
	shader->use();
	shader->setUniform(LubWindShader::Uniforms::projectionMatrix, context.projectionMatrix);
	shader->setUniform(LubWindShader::Uniforms::cameraMatrix, context.viewMatrix);
}

void LubWindRenderer::LubWindRenderContext::postFrame(NodeRenderContext& context)
{
	glDepthMask(true);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}