#pragma once

#include "Renderer.h"
#include <browedit/gl/Shader.h>
#include <browedit/util/Singleton.h>

namespace gl { class TextureArray; }
class RswObject;
class Gnd;
class EvilsPawEffect;
class BillboardRenderer;

class EvilsPawRenderer : public Renderer
{
public:
	class EvilsPawShader : public gl::Shader
	{
	public:
		EvilsPawShader() : gl::Shader("data/shaders/evilspaw", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				projectionMatrix,
				cameraMatrix,
				modelMatrix,
				scale,
				s_texture,
				layerIndex,
				selection,
				selectionColor,
				End
			};
		};
		void bindUniforms() override
		{
			bindUniform(Uniforms::projectionMatrix, "projectionMatrix");
			bindUniform(Uniforms::cameraMatrix, "cameraMatrix");
			bindUniform(Uniforms::s_texture, "s_texture");
			bindUniform(Uniforms::scale, "scale");
			bindUniform(Uniforms::modelMatrix, "modelMatrix");
			bindUniform(Uniforms::layerIndex, "layerIndex");
			bindUniform(Uniforms::selection, "selection");
			bindUniform(Uniforms::selectionColor, "selectionColor");
		}
	};
private:
	bool dirty = true;
	RswObject* rswObject = nullptr;
	EvilsPawEffect* evilsPawEffect = nullptr;
	BillboardRenderer* billboardRenderer = nullptr;
	inline static gl::TextureArray* texture = nullptr;
	float layerIndex = 0.0f;

public:
	Gnd* gnd;
	class EvilsPawRenderContext : public Renderer::RenderContext, public util::Singleton<EvilsPawRenderContext>
	{
	public:
		EvilsPawShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);

		EvilsPawRenderContext();
		virtual void preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers) override;
	};

	EvilsPawRenderer();
	~EvilsPawRenderer();
	virtual void render(NodeRenderContext& context);
	bool selected = false;
	void setDirty() { this->dirty = true; }
};