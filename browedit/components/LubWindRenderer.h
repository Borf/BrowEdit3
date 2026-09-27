#pragma once

#include "Renderer.h"
#include <browedit/gl/Shader.h>
#include <browedit/gl/Vertex.h>
#include <browedit/gl/VBO.h>
#include <browedit/gl/EBO.h>
#include <browedit/gl/VAO.h>
#include <browedit/gl/UBO.h>
#include <browedit/util/Singleton.h>
#include "LubSkyMap.h"

namespace gl { class Texture; }
class RswObject;
class Gnd;
class Rsw;
class LubWindEffect;
class BillboardRenderer;

namespace gl
{
	class TexturePoT;
}

class LubWindRenderer : public Renderer
{
public:
	class LubWindShader : public gl::Shader
	{
	public:
		LubWindShader() : gl::Shader("data/shaders/lubwind", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				s_texture,
				cameraMatrix,
				projectionMatrix,
				uTime,
				color,
				End
			};
		};
		void bindUniforms() override
		{
			bindUniform(Uniforms::s_texture, "s_texture");
			bindUniform(Uniforms::cameraMatrix, "cameraMatrix");
			bindUniform(Uniforms::projectionMatrix, "projectionMatrix");
			bindUniform(Uniforms::uTime, "uTime");
			bindUniform(Uniforms::color, "color");
		}
	};

	struct alignas(16) ParticleParams
	{
	public:
		float particleNum;
		float radius;
		float thickness;
		float height;
		float speed;
		float fullAngle;
		glm::vec2 rotateVector;
		glm::vec3 basePosition;
		float pad1;
	};

	struct Particle
	{
	public:
		//glm::vec3 position;
		float seed;
		float lifeStart;
	};

	class RenderInfo
	{
	public:
		gl::VBO<VertexP3T2>* vbo = nullptr;
		gl::VBO<Particle>* instance_vbo = nullptr;
		gl::EBO* ebo = nullptr;
		gl::VAO* vao = nullptr;
	};

	ParticleParams params;
	RenderInfo* renderInfo = nullptr;
	std::vector<Particle> particles;

	gl::UBO<ParticleParams>* ubo = nullptr;
private:
	bool dirty = true;
public:
	class LubWindRenderContext : public Renderer::RenderContext, public util::Singleton<LubWindRenderContext>
	{
	public:
		LubWindShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);

		LubWindRenderContext();
		virtual void preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers) override;
		virtual void postFrame(NodeRenderContext& context) override;
	};

	Gnd* gnd = nullptr;
	RswObject* rswObject = nullptr;
	LubWindEffect* lubWindEffect = nullptr;

	gl::TexturePoT* texture = nullptr;
	bool atlasLoaded = false;
	bool enabled = true;
	float time = 0.0f;

	LubWindRenderer();
	~LubWindRenderer();
	virtual void render(NodeRenderContext& context);
	bool selected = false;
	void setDirty() { this->dirty = true; }
};