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

	std::vector<gl::Texture*> textures;
	Rsw* rsw;
	Gnd* gnd;
	gl::VBO<VertexP3T2>* vbo = nullptr;
	std::vector<VboIndex> vertIndices;
	bool dirty = true;
	bool viewFog = false;
	bool renderFullWater = false;

	WaterRenderer();
	~WaterRenderer();
	void render() override;
	void reloadTextures();
	void setDirty();
};