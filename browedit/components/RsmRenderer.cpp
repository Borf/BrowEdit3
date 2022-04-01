#include "RsmRenderer.h"
#include "Rsm.h"
#include "Rsw.h"
#include "Gnd.h"
#include <browedit/Node.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/shaders/RsmShader.h>
#include <glm/glm.hpp>

RsmRenderer::RsmRenderer()
{
	renderContext = RsmRenderContext::getInstance();
	if (errorModel == nullptr)
		errorModel = new Rsm("data\\model\\box_error_01.rsm");
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
		delete ri.vbo;
		if(ri.textures.size() > 0)
			for(auto t : ri.textures)
				util::ResourceManager<gl::Texture>::unload(t);
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
			matrixCache = glm::translate(matrixCache, glm::vec3(-rsm->realbbrange.x, rsm->realbbmin.y, -rsm->realbbrange.z));
			if (rsm->version >= 0x0202)
			{
				matrixCache = glm::scale(matrixCache, glm::vec3(1, -1, 1));
				matrixCache = glm::translate(matrixCache, glm::vec3(0, rsm->realbbmax.y, 0));
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
	}
	auto shader = dynamic_cast<RsmRenderContext*>(renderContext)->shader;
	shader->setUniform(RsmShader::Uniforms::shadeType, (int)rsm->shadeType);
	shader->setUniform(RsmShader::Uniforms::modelMatrix2, matrixCache);
	//move to preframe
	glm::vec3 lightDirection(1,1,1);
	if (rsw)
	{
		lightDirection[0] = -glm::cos(glm::radians((float)rsw->light.longitude)) * glm::sin(glm::radians((float)rsw->light.latitude));
		lightDirection[1] = glm::cos(glm::radians((float)rsw->light.latitude));
		lightDirection[2] = glm::sin(glm::radians((float)rsw->light.longitude)) * glm::sin(glm::radians((float)rsw->light.latitude));
		shader->setUniform(RsmShader::Uniforms::lightAmbient, rsw->light.ambient);
		shader->setUniform(RsmShader::Uniforms::lightDiffuse, rsw->light.diffuse);
		shader->setUniform(RsmShader::Uniforms::lightIntensity, rsw->light.intensity);
	}
	shader->setUniform(RsmShader::Uniforms::lightDirection, lightDirection);

	if (selected)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(20.0f);
		glDepthMask(GL_FALSE);
		shader->setUniform(RsmShader::Uniforms::selection, 1.0f);
		renderMesh(rsm->rootMesh, glm::mat4(1.0f));

		glDepthMask(GL_TRUE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		shader->setUniform(RsmShader::Uniforms::selection, 0.25f);
		renderMesh(rsm->rootMesh, glm::mat4(1.0f));
		shader->setUniform(RsmShader::Uniforms::selection, 0.0f);
	}
	else
		renderMesh(rsm->rootMesh, glm::mat4(1.0f));
}


void RsmRenderer::initMeshInfo(Rsm::Mesh* mesh, const glm::mat4 &matrix)
{
	std::map<int, std::vector<VertexP3T2N3> > verts;
	for (size_t i = 0; i < mesh->faces.size(); i++)
	{
		for (int ii = 0; ii < 3; ii++)
			verts[mesh->faces[i].texId].push_back(VertexP3T2N3(mesh->vertices[mesh->faces[i].vertexIds[ii]], mesh->texCoords[mesh->faces[i].texCoordIds[ii]], mesh->faces[i].vertexNormals[ii]));
	}

	std::vector<VertexP3T2N3> allVerts;
	for (std::map<int, std::vector<VertexP3T2N3> >::iterator it2 = verts.begin(); it2 != verts.end(); it2++)
	{
		renderInfo[mesh->index].indices.push_back(VboIndex(it2->first, (int)allVerts.size(), (int)it2->second.size()));
		allVerts.insert(allVerts.end(), it2->second.begin(), it2->second.end());
	}
	if (!allVerts.empty())
	{
		renderInfo[mesh->index].vbo = new gl::VBO<VertexP3T2N3>();
		renderInfo[mesh->index].vbo->setData(allVerts, GL_STATIC_DRAW);
	}
	renderInfo[mesh->index].matrix = matrix * mesh->matrix1 * mesh->matrix2;
	renderInfo[mesh->index].matrixSub = matrix * mesh->matrix1;

	if (mesh->textureFiles.size() > 0) // 0x0203
		for (auto& textureFilename : mesh->textureFiles)
			renderInfo[mesh->index].textures.push_back(util::ResourceManager<gl::Texture>::load("data\\texture\\" + textureFilename));
	
	for (size_t i = 0; i < mesh->children.size(); i++)
		initMeshInfo(mesh->children[i], renderInfo[mesh->index].matrixSub);
}

void RsmRenderer::renderMesh(Rsm::Mesh* mesh, const glm::mat4& matrix)
{
	if (mesh && (!mesh->rotFrames.empty() || mesh->matrixDirty))
	{
		mesh->matrixDirty = false;
		mesh->calcMatrix1((int)floor(glfwGetTime() * 1000));
		renderInfo[mesh->index].matrix = matrix * mesh->matrix1 * mesh->matrix2;
		renderInfo[mesh->index].matrixSub = matrix * mesh->matrix1;
	}

	auto shader = dynamic_cast<RsmRenderContext*>(renderContext)->shader;

	RenderInfo& ri = renderInfo[mesh->index];
	if (ri.vbo != nullptr)
	{
		ri.vbo->bind();
		shader->setUniform(RsmShader::Uniforms::modelMatrix, ri.matrix);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), (void*)(0 * sizeof(float)));
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), (void*)(3 * sizeof(float)));
		glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), (void*)(5 * sizeof(float)));

		for (const VboIndex& it : ri.indices)
		{
			if(ri.textures.size() > 0)
				ri.textures[mesh->textures[it.texture]]->bind();
			else
				textures[mesh->textures[it.texture]]->bind();
			glDrawArrays(GL_TRIANGLES, (int)it.begin, (int)it.count);
		}
	}

	for (const auto& m : mesh->children)
		renderMesh(m, renderInfo[mesh->index].matrixSub);
}




RsmRenderer::RsmRenderContext::RsmRenderContext() : shader(util::ResourceManager<gl::Shader>::load<RsmShader>())
{
	order = 1;
	shader->use();
	shader->setUniform(RsmShader::Uniforms::s_texture, 0);
}

void RsmRenderer::RsmRenderContext::preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix)
{
	shader->use();
	shader->setUniform(RsmShader::Uniforms::projectionMatrix, projectionMatrix);
	shader->setUniform(RsmShader::Uniforms::cameraMatrix, viewMatrix);
	shader->setUniform(RsmShader::Uniforms::lightToggle, viewLighting);
	shader->setUniform(RsmShader::Uniforms::viewTextures, viewTextures);


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4); //TODO: vao
}