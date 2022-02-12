#pragma once

#include "Renderer.h"
#include <browedit/gl/Shader.h>
#include <browedit/gl/VBO.h>
#include <browedit/gl/Vertex.h>
#include <browedit/util/Singleton.h>
#include <browedit/components/Rsm.h>
#include <vector>

namespace gl { class Texture; }
class RswModel;
class RswObject;
class Gnd;
class Rsw;

class RsmRenderer : public Renderer
{
	class RsmShader : public gl::Shader
	{
	public:
		RsmShader() : gl::Shader("data/shaders/rsm", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				projectionMatrix,
				cameraMatrix,
				modelMatrix,
				modelMatrix2,
				s_texture,
				billboard,
				lightDiffuse,
				lightAmbient,
				lightIntensity,
				lightDirection,
				selection,
				shadeType,
				lightToggle,
				viewTextures,
				End
			};
		};
		void bindUniforms() override
		{
			bindUniform(Uniforms::projectionMatrix, "projectionMatrix");
			bindUniform(Uniforms::cameraMatrix, "cameraMatrix");
			bindUniform(Uniforms::s_texture, "s_texture");
			bindUniform(Uniforms::modelMatrix, "modelMatrix");
			bindUniform(Uniforms::modelMatrix2, "modelMatrix2");
			bindUniform(Uniforms::lightAmbient, "lightAmbient");
			bindUniform(Uniforms::lightDiffuse, "lightDiffuse");
			bindUniform(Uniforms::lightIntensity, "lightIntensity");
			bindUniform(Uniforms::lightDirection, "lightDirection");
			bindUniform(Uniforms::selection, "selection");
			bindUniform(Uniforms::shadeType, "shadeType");
			bindUniform(Uniforms::lightToggle, "lightToggle");
			bindUniform(Uniforms::viewTextures, "viewTextures");
		}
	};
public:
	class RsmRenderContext : public Renderer::RenderContext, public util::Singleton<RsmRenderContext>
	{
	public:
		RsmShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		bool viewLighting = true;
		bool viewTextures = true;

		RsmRenderContext();
		virtual void preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) override;
	};
	class VboIndex
	{
	public:
		int texture;
		int begin;
		int count;
		VboIndex(int texture, int begin, int count)
		{
			this->texture = texture;
			this->begin = begin;
			this->count = count;
		}
	};
	class RenderInfo
	{
	public:
		gl::VBO<VertexP3T2N3>* vbo;
		std::vector<VboIndex> indices;
		glm::mat4 matrix = glm::mat4(1.0f);
		glm::mat4 matrixSub = glm::mat4(1.0f);
	};

	std::vector<RenderInfo> renderInfo; //TODO: not happy about this one

	Rsm* rsm;
	RswModel* rswModel;
	RswObject* rswObject;
	Gnd* gnd;
	Rsw* rsw;

	std::vector<gl::Texture*> textures; //should this be shared over all RsmRenderers with the same Rsm? static map<Rsm, std::vector<Texture*> ???
	bool matrixCached = false;
public:
	RsmRenderer();
	void begin();
	virtual void render();
	void initMeshInfo(Rsm::Mesh* mesh, const glm::mat4& matrix = glm::mat4(1.0f));
	void renderMesh(Rsm::Mesh* mesh, const glm::mat4& matrix);
	


	void setDirty() { this->matrixCached = false; }
	bool selected = false;
	glm::mat4 matrixCache = glm::mat4(1.0f); //TODO: move this to RswObject ?
};