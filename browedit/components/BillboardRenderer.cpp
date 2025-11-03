#include "BillboardRenderer.h"
#include <browedit/util/ResourceManager.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/NodeRenderer.h>
#include <glad/gl.h>
#include <browedit/gl/Texture.h>
#include <browedit/gl/Vertex.h>

#include <glm/gtc/matrix_transform.hpp>

static std::vector<VertexP3T2> verts;

BillboardRenderer::BillboardRenderer(const std::string& textureFile, const std::string& textureFile_selected)
{
	texture = util::ResourceManager<gl::Texture>::load(textureFile);
	if(textureFile_selected != "")
		textureSelected = util::ResourceManager<gl::Texture>::load(textureFile_selected);
	renderContext = BillboardRenderContext::getInstance();
	if (verts.size() == 0)
	{
		verts.push_back(VertexP3T2(glm::vec3(-5, -5, 0), glm::vec2(0, 0)));
		verts.push_back(VertexP3T2(glm::vec3(-5, 5, 0), glm::vec2(0, 1)));
		verts.push_back(VertexP3T2(glm::vec3(5, 5, 0), glm::vec2(1, 1)));
		verts.push_back(VertexP3T2(glm::vec3(5, -5, 0), glm::vec2(1, 0)));
	}
}

BillboardRenderer::~BillboardRenderer()
{
	util::ResourceManager<gl::Texture>::unload(texture);
	if(textureSelected)
		util::ResourceManager<gl::Texture>::unload(textureSelected);
}


void BillboardRenderer::render(NodeRenderContext& context)
{
	if (!rswObject)
		rswObject = node->getComponent<RswObject>();
	if (!gnd)
		gnd = node->root->getComponent<Gnd>();
	if (!rswObject)
		return;

	auto shader = dynamic_cast<BillboardRenderContext*>(renderContext)->shader;//TODO: don't cast

	glm::mat4 modelMatrix(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1, 1, -1));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));
	shader->setUniform(BillboardShader::Uniforms::modelMatrix, modelMatrix);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (selected)
		textureSelected->bind();
	else
		texture->bind();
	glDepthMask(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), verts[0].data);
	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), verts[0].data + 3);
	glDrawArrays(GL_QUADS, 0, 4);
	glDepthMask(1);


}

void BillboardRenderer::setTexture(const std::string &texture)
{
	util::ResourceManager<gl::Texture>::unload(this->texture);
	util::ResourceManager<gl::Texture>::unload(this->textureSelected);
	this->texture = util::ResourceManager<gl::Texture>::load(texture);
	this->texture = util::ResourceManager<gl::Texture>::load(texture);
	this->textureSelected = util::ResourceManager<gl::Texture>::load(texture);
}



BillboardRenderer::BillboardRenderContext::BillboardRenderContext() : shader(util::ResourceManager<gl::Shader>::load<BillboardShader>())
{
	shader->use();
	shader->setUniform(BillboardShader::Uniforms::s_texture, 0);
	order = 3;
}

void BillboardRenderer::BillboardRenderContext::preFrame(Node* rootNode, NodeRenderContext& context)
{
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	shader->use();
	shader->setUniform(BillboardShader::Uniforms::projectionMatrix, context.projectionMatrix);
	shader->setUniform(BillboardShader::Uniforms::cameraMatrix, context.viewMatrix);
	shader->setUniform(BillboardShader::Uniforms::color, glm::vec4(1,1,1,1));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4); //TODO: vao
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
