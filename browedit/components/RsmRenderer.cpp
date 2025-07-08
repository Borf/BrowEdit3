#include "RsmRenderer.h"
#include "Rsm.h"
#include "Rsw.h"
#include "Gnd.h"
#include <browedit/Node.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/shaders/RsmShader.h>
#include <browedit/util/ResourceManager.h>
#include <glm/glm.hpp>

RsmRenderer::RsmRenderer()
{
	renderContext = RsmRenderContext::getInstance();
	if (errorModel == nullptr)
		errorModel = new Rsm("data\\model\\box_error_01.rsm");
	begin();
}

RsmRenderer::~RsmRenderer()
{
	begin();
}

void RsmRenderer::begin()
{
	selected = false;
	rsm = nullptr;
	rswModel = nullptr;
	rswObject = nullptr;
	gnd = nullptr;
	rsw = nullptr;
	matrixCached = false;
	for (auto t : textures)
		util::ResourceManager<gl::Texture>::unload(t);
	textures.clear();
	for (auto ri : renderInfo)
	{
		for (int i = 0; i < renderContext->phases; i++)
			delete ri.vbo[i];
	}
	renderInfo.clear();
}

void RsmRenderer::render()
{
	if (!this->rsm || !this->rsm->loaded)
	{
		this->rsm = node->getComponent<Rsm>();
		if (this->rsm && this->rsm->loaded)
		{//init
			for (const auto& textureFilename : rsm->textures)
				textures.push_back(util::ResourceManager<gl::Texture>::load("data\\texture\\" + textureFilename));
			
			renderInfo.resize(rsm->meshCount);
			for (int i = 0; i < rsm->meshCount; i++) {
				renderInfo[i].vbo.resize(renderContext->phases);
				renderInfo[i].indices.resize(renderContext->phases);
			}
			initMeshInfo(rsm->rootMesh);
		}
	}
	if (!this->rswModel)
		this->rswModel = node->getComponent<RswModel>();
	if (!this->rswObject)
		this->rswObject = node->getComponent<RswObject>();
	if (!this->gnd)
		this->gnd = node->root->getComponent<Gnd>(); //TODO: remove parent->parent
	if (!this->rsw)
		this->rsw = node->root->getComponent<Rsw>();
	if (!this->rsm)
		return;

	if (!this->rsm->loaded)
	{
		this->rsm = RsmRenderer::errorModel;
		for (const auto& textureFilename : rsm->textures)
			textures.push_back(util::ResourceManager<gl::Texture>::load("data\\texture\\" + textureFilename));
		renderInfo.resize(rsm->meshCount);
		for (int i = 0; i < rsm->meshCount; i++) {
			renderInfo[i].vbo.resize(renderContext->phases);
			renderInfo[i].indices.resize(renderContext->phases);
		}
		initMeshInfo(rsm->rootMesh);
	}

	if (meshDirty)
	{
		for (auto ri : renderInfo)
		{
			for (int i = 0; i < renderContext->phases; i++)
				delete ri.vbo[i];
		}
		renderInfo.clear();
		renderInfo.resize(rsm->meshCount);
		for (int i = 0; i < rsm->meshCount; i++) {
			renderInfo[i].vbo.resize(renderContext->phases);
			renderInfo[i].indices.resize(renderContext->phases);
		}
		initMeshInfo(rsm->rootMesh);
	}

	if (!matrixCached && this->rswModel && this->gnd && this->rsw)
	{
		matrixCache = glm::mat4(1.0f);
		matrixCache = glm::scale(matrixCache, glm::vec3(1, 1, -1));
		matrixCache = glm::translate(matrixCache, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));
		matrixCache = glm::rotate(matrixCache, -glm::radians(rswObject->rotation.z), glm::vec3(0, 0, 1));
		matrixCache = glm::rotate(matrixCache, -glm::radians(rswObject->rotation.x), glm::vec3(1, 0, 0));
		matrixCache = glm::rotate(matrixCache, glm::radians(rswObject->rotation.y), glm::vec3(0, 1, 0));
		if (rswModel)
		{
			matrixCache = glm::scale(matrixCache, glm::vec3(rswObject->scale.x, -rswObject->scale.y, rswObject->scale.z));

			if (rsm->version < 0x0202) {
				matrixCache = glm::translate(matrixCache, glm::vec3(-rsm->realbbrange.x, rsm->realbbmin.y, -rsm->realbbrange.z));
			}
			else {
				matrixCache = glm::scale(matrixCache, glm::vec3(1, -1, 1));
			}

		}
		matrixCached = true;

		if (rswModel)
		{
			std::vector<glm::vec3> verts = math::AABB::boxVerts(rsm->realbbmin, rsm->realbbmax);
			for (size_t i = 0; i < verts.size(); i++)
				verts[i] = glm::vec3(matrixCache * glm::vec4(glm::vec3(1, -1, 1) * verts[i], 1.0f));
			rswModel->aabb.min = glm::vec3(99999999, 99999999, 99999999);
			rswModel->aabb.max = glm::vec3(-99999999, -99999999, -99999999);
			for (size_t i = 0; i < verts.size(); i++)
			{
				rswModel->aabb.min = glm::min(rswModel->aabb.min, verts[i]);
				rswModel->aabb.max = glm::max(rswModel->aabb.max, verts[i]);
			}
		}

		reverseCullFace = false;

		if (rswObject->scale.x * rswObject->scale.y * rswObject->scale.z * (rsm->version >= 0x202 ? -1 : 1) < 0)
			reverseCullFace = true;
	}

	if (!matrixCached && rsm->version >= 0x0202)
	{
		matrixCache = glm::mat4(1.0f);
		matrixCache = glm::scale(matrixCache, glm::vec3(1, -1, 1));
	}

	phase = dynamic_cast<RsmRenderContext*>(renderContext)->phase;

	auto shader = dynamic_cast<RsmRenderContext*>(renderContext)->shader;
	shader->setUniform(RsmShader::Uniforms::shadeType, (int)rsm->shadeType);
	shader->setUniform(RsmShader::Uniforms::modelMatrix2, matrixCache);

	if (reverseCullFace)
		glFrontFace(GL_CW);
	else
		glFrontFace(GL_CCW);

	if (selected)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(20.0f);
		glDepthMask(GL_FALSE);
		shader->setUniform(RsmShader::Uniforms::selection, 1.0f);
		renderMesh(rsm->rootMesh, glm::mat4(1.0f), true);
		
		glDepthMask(GL_TRUE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (phase > 0) {
			glDepthMask(GL_FALSE);
		}

		shader->setUniform(RsmShader::Uniforms::selection, 0.25f);
		renderMesh(rsm->rootMesh, glm::mat4(1.0f));
		shader->setUniform(RsmShader::Uniforms::selection, 0.0f);
	}
	else
		renderMesh(rsm->rootMesh, glm::mat4(1.0f));
}


void RsmRenderer::initMeshInfo(Rsm::Mesh* mesh, const glm::mat4 &matrix)
{
	std::map<int, std::vector<VertexP3T2N3C1>> verts;

	for (size_t i = 0; i < mesh->faces.size(); i++)
	{
		for (int ii = 0; ii < 3; ii++)
			verts[mesh->faces[i].texId].push_back(VertexP3T2N3C1(mesh->vertices[mesh->faces[i].vertexIds[ii]], mesh->texCoords[mesh->faces[i].texCoordIds[ii]], mesh->faces[i].vertexNormals[ii], mesh->faces[i].twoSided));
	}

	std::vector<VertexP3T2N3C1> allVerts[3];
	int phaseId = 0;

	for (std::map<int, std::vector<VertexP3T2N3C1> >::iterator it2 = verts.begin(); it2 != verts.end(); it2++)
	{
		if (textures[mesh->textures[it2->first]]->semiTransparent) {
			// animated semiTransparent texture
			if (mesh->textureFrames.find(it2->first) != mesh->textureFrames.end()) {
				phaseId = 2;
			}
			else {
				phaseId = 1;
			}
		}
		else {
			phaseId = 0;
		}
		
		renderInfo[mesh->index].indices[phaseId].push_back(VboIndex(it2->first, (int)allVerts[phaseId].size(), (int)it2->second.size()));
		allVerts[phaseId].insert(allVerts[phaseId].end(), it2->second.begin(), it2->second.end());
	}

	for (int i = 0; i < renderContext->phases; i++) {
		if (!allVerts[i].empty()) {
			renderInfo[mesh->index].vbo[i] = new gl::VBO<VertexP3T2N3C1>();
			renderInfo[mesh->index].vbo[i]->setData(allVerts[i], GL_STATIC_DRAW);
		}
	}

	if (mesh->model->version >= 0x202) {
		renderInfo[mesh->index].matrix = mesh->matrix2;
		renderInfo[mesh->index].matrixSub = glm::mat4(1.0f);
	}
	else {
		renderInfo[mesh->index].matrix = matrix * mesh->matrix1 * mesh->matrix2;
		renderInfo[mesh->index].matrixSub = matrix * mesh->matrix1;
	}

	for (size_t i = 0; i < mesh->children.size(); i++)
		initMeshInfo(mesh->children[i], renderInfo[mesh->index].matrixSub);
	meshDirty = false;
}

void RsmRenderer::renderMeshSub(Rsm::Mesh* mesh, bool selectionPhase)
{
	if (phase >= 3)
		return;

	auto shader = dynamic_cast<RsmRenderContext*>(renderContext)->shader;
	RenderInfo& ri = renderInfo[mesh->index];
	gl::VBO<VertexP3T2N3C1>* vbo = renderInfo[mesh->index].vbo[phase];
	std::vector<VboIndex>* indices = &renderInfo[mesh->index].indices[phase];

	if (vbo != nullptr)
	{
		vbo->bind();
		shader->setUniform(RsmShader::Uniforms::modelMatrix, renderInfo[mesh->index].matrix);
		if (ri.selected || selectionPhase)
			shader->setUniform(RsmShader::Uniforms::selection, 1.0f);
		else if (!selectionPhase && selected)
			shader->setUniform(RsmShader::Uniforms::selection, .25f);
		else
			shader->setUniform(RsmShader::Uniforms::selection, .0f);
		if (ri.selected)
			glDisable(GL_DEPTH_TEST);

		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3C1), (void*)(0 * sizeof(float)));
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3C1), (void*)(3 * sizeof(float)));
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3C1), (void*)(5 * sizeof(float)));
		glVertexAttribPointer(3, 1, GL_FLOAT, false, sizeof(VertexP3T2N3C1), (void*)(8 * sizeof(float)));

		for (const VboIndex& it : *indices)
		{
			bool isRepeat = false;

			if (mesh->model->version >= 0x203) {
				glm::vec2 texTranslate(0.0f);
				glm::vec3 texScale(1.0f);
				glm::mat4 texMat(1.0f);
				glm::vec3 centerOffset(0.5f, 0.5f, 0.0f);
				float texRot = 0.0f;
				float mult = this->rswModel != nullptr ? this->rswModel->animSpeed : 1.0f;
				int tick = time < 0 ? (int)floor(glfwGetTime() * 1000 * mult) : (int)floor(time * 1000 * mult);
				texMat = glm::translate(texMat, centerOffset);

				if (mesh->getTextureAnimation(it.texture, tick, texTranslate, texScale, texRot)) {
					isRepeat = true;
					texMat = glm::scale(texMat, texScale);
					texMat = glm::rotate(texMat, texRot, glm::vec3(0, 0, 1));
					texMat[3][0] += texTranslate.x;
					texMat[3][1] += texTranslate.y;
					texMat = glm::translate(texMat, -centerOffset);
					shader->setUniform(RsmShader::Uniforms::texMat, texMat);
				}
			}

			textures[mesh->textures[it.texture]]->bind();

			if (isRepeat) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}

			glDrawArrays(GL_TRIANGLES, (int)it.begin, (int)it.count);

			if (isRepeat) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
			}
		}
		if (ri.selected)
			glEnable(GL_DEPTH_TEST);
	}
}

void RsmRenderer::renderMesh(Rsm::Mesh* mesh, const glm::mat4& matrix, bool selectionPhase, bool calcMatrix)
{
	if (textures.empty())
	{
		for (const auto& textureFilename : rsm->textures)
			textures.push_back(util::ResourceManager<gl::Texture>::load("data\\texture\\" + textureFilename));
	}

	if (phase == 0 && mesh && mesh->isAnimated)
	{
		if (calcMatrix) {
			calcMatrix = false;
			float mult = this->rswModel != nullptr ? this->rswModel->animSpeed : 1.0f;
			
			if(time < 0)
				mesh->calcMatrix1((int)floor(glfwGetTime() * 1000 * mult));
			else
				mesh->calcMatrix1((int)floor(time * 1000 * mult));
		}

		if (mesh->model->version >= 0x202) {
			renderInfo[mesh->index].matrix = mesh->matrix2;
			renderInfo[mesh->index].matrixSub = glm::mat4(1.0f);
		}
		else {
			renderInfo[mesh->index].matrix = matrix * mesh->matrix1 * mesh->matrix2;
			renderInfo[mesh->index].matrixSub = matrix * mesh->matrix1;
		}
	}

	renderMeshSub(mesh, selectionPhase);

	for (const auto& m : mesh->children)
		renderMesh(m, renderInfo[mesh->index].matrixSub, selectionPhase, calcMatrix);
}

void RsmRenderer::setMeshesDirty() {
	this->meshDirty = true;
	this->matrixCached = false;
	if(rsm)
		rsm->updateMatrices();
	for (auto t : textures)
		util::ResourceManager<gl::Texture>::unload(t);
	textures.clear();
}




RsmRenderer::RsmRenderContext::RsmRenderContext() : shader(util::ResourceManager<gl::Shader>::load<RsmShader>())
{
	order = 1;
	phases = 3;
	shader->use();
	shader->setUniform(RsmShader::Uniforms::s_texture, 0);
}

void RsmRenderer::RsmRenderContext::preFrame(Node* rootNode, const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
{
	shader->use();

	if (phase == 0) {
		shader->setUniform(RsmShader::Uniforms::projectionMatrix, projectionMatrix);
		shader->setUniform(RsmShader::Uniforms::cameraMatrix, viewMatrix);
		shader->setUniform(RsmShader::Uniforms::lightToggle, viewLighting);
		shader->setUniform(RsmShader::Uniforms::viewTextures, viewTextures);
		shader->setUniform(RsmShader::Uniforms::fogEnabled, viewFog);
		shader->setUniform(RsmShader::Uniforms::enableCullFace, enableFaceCulling);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glDisableVertexAttribArray(4); //TODO: vao	

		Rsw* rsw = rootNode->getComponent<Rsw>();

		glm::vec3 lightDirection(1, 1, 1);
		if (rsw) {
			glm::mat4 rot = glm::mat4(1.0f);
			rot = glm::rotate(rot, glm::radians(-(float)rsw->light.latitude), glm::vec3(1, 0, 0));
			rot = glm::rotate(rot, glm::radians((float)rsw->light.longitude), glm::vec3(0, 1, 0));
			lightDirection = glm::vec3(0.0f, 1.0f, 0.0f) * glm::mat3(rot);
			shader->setUniform(RsmShader::Uniforms::lightAmbient, rsw->light.ambient);
			shader->setUniform(RsmShader::Uniforms::lightDiffuse, rsw->light.diffuse);

			shader->setUniform(RsmShader::Uniforms::fogNear, rsw->fog.nearPlane * 240 * 2.5f);
			shader->setUniform(RsmShader::Uniforms::fogFar, rsw->fog.farPlane * 240 * 2.5f);
			//shader->setUniform(RsmShader::Uniforms::fogExp, rsw->fog.factor);
			shader->setUniform(RsmShader::Uniforms::fogColor, rsw->fog.color);
		}
		shader->setUniform(RsmShader::Uniforms::lightDirection, lightDirection);
	}
	
	if (phase == 0) {
		shader->setUniform(RsmShader::Uniforms::discardAlphaValue, 0.8f);
		glDepthMask(true);
		glDisable(GL_BLEND);
	}
	else if (phase == 1) {
		shader->setUniform(RsmShader::Uniforms::discardAlphaValue, 0.0f);
		glDepthMask(false);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (phase == 2) {
		shader->setUniform(RsmShader::Uniforms::discardAlphaValue, 0.0f);
		glDepthMask(false);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		shader->setUniform(RsmShader::Uniforms::textureAnimToggle, true);

		shader->setUniform(RsmShader::Uniforms::enableCullFace, false);
	}
}

void RsmRenderer::RsmRenderContext::postFrame()
{
	if (phase == 0) {

	}
	else if (phase == 1) {
		glDepthMask(true);
	}
	else if (phase == 2) {
		glDepthMask(true);
		shader->setUniform(RsmShader::Uniforms::textureAnimToggle, false);
	}

	glFrontFace(GL_CCW);
}