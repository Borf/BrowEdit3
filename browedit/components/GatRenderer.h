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
class Gat;
class Rsw;
class SimpleShader;

#define CHUNKSIZE 16

class GatRenderer : public Renderer
{
public:
	class GatRenderContext : public Renderer::RenderContext, public util::Singleton<GatRenderContext>
	{
	public:
		SimpleShader* shader = nullptr;
		gl::Texture* texture = nullptr;
		GatRenderContext();
		virtual void preFrame(Node* rootNode, NodeRenderContext& context) override;
	};

	class Chunk
	{
	public:
		bool dirty;
		bool rebuilding;
		gl::VBO<VertexP3T2N3> vbo;
		int x, y;
		GatRenderer* renderer;
		Gat* gat;

		Chunk(int x, int y, Gat* gnd, GatRenderer* renderer);
		~Chunk();
		void render();
		void rebuild();
	};

	float cameraDistance = 100;
	float opacity = 0.5f;
	std::vector<std::vector<Chunk*> > chunks; //TODO: remove pointer?
	bool allDirty = true;
	Gat* gat;

	void setChunkDirty(int x, int y);
	void setChunksDirty();

	GatRenderer(gl::Texture* texture);
	~GatRenderer();
	void render(NodeRenderContext& context) override;
};