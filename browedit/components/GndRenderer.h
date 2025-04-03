#pragma once

#include "Renderer.h"
#include <browedit/gl/Shader.h>
#include <browedit/util/Singleton.h>
#include <browedit/gl/Vertex.h>
#include <browedit/gl/VBO.h>

#include <vector>

namespace gl
{
	class Texture;
}
class Gnd;
class Rsw;
class GndShader;

#define CHUNKSIZE 16

class GndRenderer : public Renderer
{
public:
	int shadowmapSize;

	class GndRenderContext : public Renderer::RenderContext, public util::Singleton<GndRenderContext>
	{
	public:
		GndShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);

		GndRenderContext();
		virtual void preFrame(Node* rootNode, const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) override;
	};

	class VboIndex
	{
	public:
		int texture;
		std::size_t begin;
		std::size_t count;
		VboIndex(int texture, std::size_t begin, std::size_t count)
		{
			this->texture = texture;
			this->begin = begin;
			this->count = count;
		}
	};

	class Chunk
	{
	public:
		bool dirty;
		bool rebuilding;
		gl::VBO<VertexP3T2T2C4N3> vbo;
		std::vector<VboIndex> vertIndices;
		int x, y;
		GndRenderer* renderer;
		Gnd* gnd;

		Chunk(int x, int y, Gnd* gnd, GndRenderer* renderer);
		~Chunk();
		void render();
		void rebuild();
	};

	gl::Texture* white;
	std::vector<gl::Texture*> textures;
	std::vector<std::vector<Chunk*> > chunks; //TODO: remove pointer?
	bool allDirty = true;
	Gnd* gnd;
	Rsw* rsw; //for lighting

	gl::Texture* gndShadow = nullptr;

	void setChunkDirty(int x, int y);
	void setChunksDirty();
	void rebuildChunks();
	bool gndShadowDirty = true;

	GndRenderer();
	~GndRenderer();
	void render() override;


	bool viewLightmapShadow = true;
	bool viewLightmapColor = true;
	bool viewColors = true;
	bool viewLighting = true;
	bool smoothColors = false;
	bool viewTextures = true;
	bool viewFog = true;
	bool quickRenderLight_hideOthers = false;
	Node* quickRenderLightNode = nullptr;

	bool viewEmptyTiles = true;
};