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
class WaterShader;

class WaterRenderer : public Renderer
{
public:
	class WaterRenderContext : public Renderer::RenderContext, public util::Singleton<WaterRenderContext>
	{
	public:
		WaterShader* shader = nullptr;

		WaterRenderContext();
		virtual void preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) override;
	};

	std::vector<gl::Texture*> textures;
	Rsw* rsw;
	gl::VBO<VertexP3T2>* vbo = nullptr;
	bool dirty = true;
	bool viewFog = false;

	WaterRenderer();
	~WaterRenderer();
	void render() override;
	void reloadTextures();
	void setDirty();
};