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
class RsmShader;

class RsmRenderer : public Renderer
{
public:
	class RsmRenderContext : public Renderer::RenderContext, public util::Singleton<RsmRenderContext>
	{
	public:
		RsmShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		bool viewLighting = true;
		bool viewTextures = true;
		bool viewFog = true;

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
		std::vector<gl::Texture*> textures; //should this be shared over all RenderInfo with the same RsmMesh?
	};

	std::vector<RenderInfo> renderInfo; //TODO: not happy about this one

	Rsm* rsm;
	RswModel* rswModel;
	RswObject* rswObject;
	Gnd* gnd;
	Rsw* rsw;

	float time = -1;
	bool meshDirty = true;

	std::vector<gl::Texture*> textures; //should this be shared over all RsmRenderers with the same Rsm? static map<Rsm, std::vector<Texture*> ???
	bool matrixCached = false;

	inline static Rsm* errorModel = nullptr;

public:
	RsmRenderer();
	~RsmRenderer();
	void begin();
	virtual void render();
	void initMeshInfo(Rsm::Mesh* mesh, const glm::mat4& matrix = glm::mat4(1.0f));
	void renderMesh(Rsm::Mesh* mesh, const glm::mat4& matrix);
	

	void setMeshesDirty() { this->meshDirty = true; this->matrixCached = false; }

	void setDirty() { this->matrixCached = false; }
	bool selected = false;
	glm::mat4 matrixCache = glm::mat4(1.0f); //TODO: move this to RswObject ?
};