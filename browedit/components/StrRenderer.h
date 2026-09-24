#pragma once

#include "Renderer.h"
#include <browedit/gl/Shader.h>
#include <browedit/gl/VBO.h>
#include <browedit/gl/Vertex.h>
#include <browedit/util/Singleton.h>
#include <browedit/components/Str.h>
#include <vector>

namespace gl { class Texture; }
class RswModel;
class RswObject;
class Gnd;
class StrEffect;

class StrRenderer : public Renderer
{
public:
	class StrShader : public gl::Shader
	{
	public:
		StrShader() : gl::Shader("data/shaders/str", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				projectionMatrix,
				cameraMatrix,
				modelMatrix,
				modelPosition,
				s_texture,
				color,
				alpha,
				selection,
				End
			};
		};
		void bindUniforms() override
		{
			bindUniform(Uniforms::projectionMatrix, "projectionMatrix");
			bindUniform(Uniforms::cameraMatrix, "cameraMatrix");
			bindUniform(Uniforms::s_texture, "s_texture");
			bindUniform(Uniforms::modelMatrix, "modelMatrix");
			bindUniform(Uniforms::modelPosition, "modelPosition");
			bindUniform(Uniforms::color, "color");
			bindUniform(Uniforms::alpha, "alpha");
			bindUniform(Uniforms::selection, "selection");
		}
	};
	class StrRenderContext : public Renderer::RenderContext, public util::Singleton<StrRenderContext>
	{
	public:
		StrShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		bool viewLighting = true;
		bool viewTextures = true;
		bool viewFog = true;
		bool enableFaceCulling = true;
		int counter = 0;

		StrRenderContext();
		virtual void preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers) override;
		virtual void postFrame(NodeRenderContext& context) override;
	};

	Str* str;
	StrEffect* strEffect;
	RswObject* rswObject;
	Gnd* gnd;

	int phase = 0;
	bool dirty = true;

	std::vector<std::vector<gl::Texture*>> textures;
	std::vector<VertexP2T2> verts;
public:
	StrRenderer();
	~StrRenderer();
	void begin();
	virtual void render(NodeRenderContext& context) override;
	
	virtual bool shouldRender(int phase) { return true; }

	bool selected = false;
};