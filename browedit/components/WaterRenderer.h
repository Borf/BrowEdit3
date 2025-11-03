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
class Rsw;
class Gnd;
class WaterShader;

class WaterRenderer : public Renderer
{
public:
	class WaterRenderContext : public Renderer::RenderContext, public util::Singleton<WaterRenderContext>
	{
	public:
		WaterShader* shader = nullptr;

		WaterRenderContext();
		virtual void preFrame(Node* rootNode, NodeRenderContext& context) override;
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

	Rsw* rsw;
	Gnd* gnd;
	gl::VBO<VertexP3T2>* vbo = nullptr;
	std::vector<VboIndex> vertIndices;
	std::unordered_map<int, std::vector<gl::Texture*>> type2textures;
	bool dirty = true;
	bool viewFog = false;
	bool renderFullWater = false;

	WaterRenderer();
	~WaterRenderer();
	void render(NodeRenderContext& context) override;
	void reloadTextures();
	void setDirty();
};