#include "EvilsPawRenderer.h"
#include <browedit/util/Util.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/NodeRenderer.h>
#include <browedit/components//BillboardRenderer.h>
#include <glad/gl.h>
#include <browedit/gl/Texture.h>
#include <browedit/gl/Vertex.h>

#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

static std::vector<VertexP2T2> verts;

EvilsPawRenderer::EvilsPawRenderer()
{
	renderContext = EvilsPawRenderContext::getInstance();

	if (verts.size() == 0) {
		verts.push_back(VertexP2T2(glm::vec2(-1, -1), glm::vec2(0, 0)));
		verts.push_back(VertexP2T2(glm::vec2(-1, 1), glm::vec2(0, 1)));
		verts.push_back(VertexP2T2(glm::vec2(1, 1), glm::vec2(1, 1)));
		verts.push_back(VertexP2T2(glm::vec2(1, -1), glm::vec2(1, 0)));
	}
}

EvilsPawRenderer::~EvilsPawRenderer()
{
}

void EvilsPawRenderer::render(NodeRenderContext& context)
{
	if (!rswObject)
		rswObject = node->getComponent<RswObject>();
	if (!gnd)
		gnd = node->root->getComponent<Gnd>();
	if (!billboardRenderer)
		billboardRenderer = node->getComponent<BillboardRenderer>();
	
	if (!evilsPawEffect || dirty || evilsPawEffect->dirty)
	{
		evilsPawEffect = node->getComponent<EvilsPawEffect>();
		dirty = false;

		if (texture == nullptr) {
			texture = new gl::TextureArray(512, 512, 20);

			for (int i = 0; i < 20; i++) {
				std::string texturePath;
				
				if (i < 10)
					texturePath = "data\\texture\\effect\\1-" + std::to_string((i + 1)) + ".tga";
				else
					texturePath = "data\\texture\\effect\\2-" + std::to_string((i - 10 + 1)) + ".tga";

				std::istream* is = util::FileIO::open(texturePath);
				if (!is)
				{
					std::cerr << "Texture: Could not open " << texturePath << std::endl;
					continue;
				}
				is->seekg(0, std::ios_base::end);
				std::size_t len = is->tellg();
				if (len <= 0 || len > 100 * 1024 * 1024)
				{
					std::cerr << "Texture: Error opening texture " << texturePath << ", file is either empty or too large" << std::endl;
					delete is;
					continue;
				}

				char* buffer = new char[len];
				is->seekg(0, std::ios_base::beg);
				is->read(buffer, len);
				delete is;

				int width, height, comp;
				stbi_set_flip_vertically_on_load(true);
				unsigned char* data = stbi_load_from_memory((stbi_uc*)buffer, (int)len, &width, &height, &comp, 4);

				if (width != 512 || height != 512) {
					std::cerr << "Texture: Invalid image dimension (512x512) for " << texturePath << std::endl;
					continue;
				}

				texture->setSubImage((char*)data, 0, 0, i, 512, 512);
				stbi_image_free(data);
			}
		}

		if (evilsPawEffect)
			evilsPawEffect->dirty = false;

		layerIndex = 0;
	}

	if (!rswObject || !evilsPawEffect || !gnd)
		return;

	auto shader = dynamic_cast<EvilsPawRenderContext*>(renderContext)->shader;//TODO: don't cast

	glm::mat4 modelMatrix(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1, 1, -1));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));
	modelMatrix = glm::translate(modelMatrix, evilsPawEffect->offsetPos * glm::vec3(1, -1, 1));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	if (texture)
		texture->bind();
	else
		return;

	float time = (float)glfwGetTime();

	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(VertexP2T2), verts[0].data);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP2T2), verts[0].data + 2);

	layerIndex = glm::mod((time / glm::max(0.1f, evilsPawEffect->speed)) * (100.0f / 1.6f), 20.0f);	// 1.6f is Gravity's time dilation.

	shader->setUniform(EvilsPawShader::Uniforms::modelMatrix, modelMatrix);
	shader->setUniform(EvilsPawShader::Uniforms::scale, evilsPawEffect->size);
	shader->setUniform(EvilsPawShader::Uniforms::layerIndex, layerIndex);
	glDrawArrays(GL_QUADS, 0, 4);

	if (billboardRenderer != nullptr && billboardRenderer->selected) {
		glLineWidth(1.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		shader->setUniform(EvilsPawShader::Uniforms::selection, true);
		glDrawArrays(GL_QUADS, 0, 4);
		shader->setUniform(EvilsPawShader::Uniforms::selection, false);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}


EvilsPawRenderer::EvilsPawRenderContext::EvilsPawRenderContext() : shader(util::ResourceManager<gl::Shader>::load<EvilsPawShader>())
{
	shader->use();
	shader->setUniform(EvilsPawShader::Uniforms::s_texture, 0);
	order = RendererDrawPriority::Lub;
}

void EvilsPawRenderer::EvilsPawRenderContext::preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers)
{
	glEnable(GL_DEPTH_TEST);
	shader->use();
	shader->setUniform(EvilsPawShader::Uniforms::projectionMatrix, context.projectionMatrix);
	shader->setUniform(EvilsPawShader::Uniforms::cameraMatrix, context.viewMatrix);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDepthMask(0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
