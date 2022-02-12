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

#define CHUNKSIZE 16

class GndRenderer : public Renderer
{
public:
	class GndShader : public gl::Shader
	{
	public:
		GndShader() : gl::Shader("data/shaders/gnd", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				ProjectionMatrix,
				ModelViewMatrix,
				s_texture,
				s_lighting,
				s_tileColor,
				lightAmbient,
				lightDiffuse,
				lightIntensity,
				lightDirection,
				shadowMapToggle,
				lightToggle,
				colorToggle,
				lightColorToggle,
				viewTextures,
				End
			};
		};
		void bindUniforms() override
		{
			bindUniform(Uniforms::ProjectionMatrix, "projectionMatrix");
			bindUniform(Uniforms::ModelViewMatrix, "modelViewMatrix");
			bindUniform(Uniforms::s_texture, "s_texture");
			bindUniform(Uniforms::s_lighting, "s_lighting");
			bindUniform(Uniforms::s_tileColor, "s_tileColor");
			bindUniform(Uniforms::lightAmbient, "lightAmbient");
			bindUniform(Uniforms::lightDiffuse, "lightDiffuse");
			bindUniform(Uniforms::lightIntensity, "lightIntensity");
			bindUniform(Uniforms::lightDirection, "lightDirection");
			bindUniform(Uniforms::shadowMapToggle, "shadowMapToggle");
			bindUniform(Uniforms::lightToggle, "lightToggle");
			bindUniform(Uniforms::colorToggle, "colorToggle");
			bindUniform(Uniforms::lightColorToggle, "lightColorToggle");
			bindUniform(Uniforms::viewTextures, "viewTextures");			
		}
	};
	class GndRenderContext : public Renderer::RenderContext, public util::Singleton<GndRenderContext>
	{
	public:
		GndShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);

		GndRenderContext();
		virtual void preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) override;
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
		gl::VBO<VertexP3T2T2T2N3> vbo;
		std::vector<VboIndex> vertIndices;
		int x, y;
		GndRenderer* renderer;
		Gnd* gnd;

		Chunk(int x, int y, Gnd* gnd, GndRenderer* renderer);

		void render();
		void rebuild();
	};

	std::vector<gl::Texture*> textures;
	std::vector<std::vector<Chunk*> > chunks; //TODO: remove pointer?
	Gnd* gnd;
	Rsw* rsw; //for lighting

	gl::Texture* gndShadow;
	gl::Texture* gndTileColors;


	void setChunkDirty(int x, int y);
	bool gndShadowDirty = true;
	bool gndTileColorDirty = true;

	GndRenderer();
	void render() override;


	bool viewLightmapShadow = true;
	bool viewLightmapColor = true;
	bool viewColors = true;
	bool viewLighting = true;
	bool smoothColors = false;
	bool viewTextures = true;
};