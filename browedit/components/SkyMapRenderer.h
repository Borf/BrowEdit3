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
class LubEffect;
class BillboardRenderer;

class SkyMapRenderer : public Renderer
{
public:
	class SkyMapShader : public gl::Shader
	{
	public:
		SkyMapShader() : gl::Shader("data/shaders/skymap", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				s_texture,
				cameraMatrix,
				projectionMatrix,
				uTime,
				color,
				aForcedDir,
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
		float Size;
		float Size_Extra;
		float Height;
		float Height_Extra;
		float Alpha_Inc_Time;
		float Alpha_Inc_Time_Extra;
		float Alpha_Inc_Speed;
		float Alpha_Dec_Time;
		float Alpha_Dec_Time_Extra;
		float Alpha_Dec_Speed;
		float Expand_Rate;
		float uvSplit;
		float uvCycleSpeed;
		float scaleX;
		float scaleY;
		int dirMode;
		glm::vec4 forcedDir;
	};

	struct Particle
	{
	public:
		glm::vec3 position;
		float seed;
		float lifeStart;
		float lifeEnd;
		float alphaDecTime;
		float expandDelay;
		float uvStart;
	};

	class RenderInfo
	{
	public:
		gl::VBO<VertexP3T2>* vbo = nullptr;
		gl::VBO<Particle>* instance_vbo = nullptr;
		gl::EBO* ebo = nullptr;
		gl::VAO* vao = nullptr;
	};

	struct CloudInstance {
		const LubSkyMap::CloudEffect* source;

		ParticleParams params;
		gl::Texture* textureAtlas;
		RenderInfo* renderInfo = nullptr;
		std::vector<Particle> particles;
	};

	gl::UBO<ParticleParams>* ubo = nullptr;
	std::vector<CloudInstance> cloudInstances;
private:
	bool dirty = true;
public:
	class SkyMapRenderContext : public Renderer::RenderContext, public util::Singleton<SkyMapRenderContext>
	{
	public:
		SkyMapShader* shader = nullptr;
		glm::mat4 viewMatrix = glm::mat4(1.0f);

		SkyMapRenderContext();
		virtual void preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers) override;
		virtual void postFrame(NodeRenderContext& context) override;
	};

	Gnd* gnd = nullptr;
	Rsw* rsw = nullptr;
	LubSkyMap* lubSkyMap = nullptr;

	gl::Texture* cloudAtlas = nullptr;
	gl::Texture* starAtlas = nullptr;
	gl::Texture* fogAtlas = nullptr;
	bool atlasLoaded = false;
	bool enabled = true;
	float time = 0.0f;

	SkyMapRenderer();
	~SkyMapRenderer();
	virtual void render(NodeRenderContext& context);
	bool selected = false;
	void setDirty() { this->dirty = true; }

	gl::Texture* createTextureAtlas(std::initializer_list<std::string> textures);
	void updateParticle(SkyMapRenderer::CloudInstance& cloudInstance, SkyMapRenderer::Particle& particle, int i);
	void reload();
};