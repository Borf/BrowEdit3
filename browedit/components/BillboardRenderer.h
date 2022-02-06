#pragma once

#include "Renderer.h"
#include <browedit/gl/Shader.h>
#include <browedit/util/Singleton.h>

namespace gl { class Texture; }
class RswObject;
class Gnd;

class BillboardRenderer : public Renderer
{
	class BillboardShader : public gl::Shader
	{
	public:
		BillboardShader() : gl::Shader("data/shaders/billboard", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				projectionMatrix,
				cameraMatrix,
				modelMatrix,
				s_texture,
				billboard,
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
			bindUniform(Uniforms::selection, "selection");
		}
	};
	RswObject* rswObject;
	gl::Texture* texture;
	gl::Texture* textureSelected = nullptr;

public:
	Gnd* gnd;
	class BillboardRenderContext : public Renderer::RenderContext, public util::Singleton<BillboardRenderContext>
	{
	public:
		BillboardShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		bool viewLighting = true;

		BillboardRenderContext();
		virtual void preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) override;
	};

	BillboardRenderer(const std::string& texture, const std::string& texture_selected = "");
	virtual void render();
	bool selected = false;

};