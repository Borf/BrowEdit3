#pragma once

#include "Renderer.h"
#include <browedit/gl/Shader.h>
#include <browedit/util/Singleton.h>

namespace gl { class Texture; }
class RswObject;
class Gnd;
class NodeRenderContext;

class BillboardRenderer : public Renderer
{
public:
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
				color,
				billboard,
				End
			};
		};
		void bindUniforms() override
		{
			bindUniform(Uniforms::projectionMatrix, "projectionMatrix");
			bindUniform(Uniforms::cameraMatrix, "cameraMatrix");
			bindUniform(Uniforms::s_texture, "s_texture");
			bindUniform(Uniforms::modelMatrix, "modelMatrix");
			bindUniform(Uniforms::color, "color");
		}
	};
private:
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
		virtual void preFrame(Node* rootNode, NodeRenderContext& context) override;
	};

	BillboardRenderer(const std::string& texture, const std::string& texture_selected = "");
	~BillboardRenderer();
	virtual void render(NodeRenderContext& context);
	bool selected = false;

	void setTexture(const std::string &texture);

};