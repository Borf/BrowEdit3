#pragma once

#include <glm/glm.hpp>
#include <browedit/gl/Shader.h>
#include <browedit/gl/Vertex.h>
#include <browedit/gl/VBO.h>
#include "Mesh.h"

#include <vector>

class Gadget
{
public:
	class SimpleShader : public gl::Shader
	{
	public:
		SimpleShader() : gl::Shader("data/shaders/simple", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				projectionMatrix,
				viewMatrix,
				modelMatrix,
				s_texture,
				textureFac,
				color,
				End
			};
		};
		void bindUniforms() override
		{
			bindUniform(Uniforms::projectionMatrix, "projectionMatrix");
			bindUniform(Uniforms::viewMatrix, "viewMatrix");
			bindUniform(Uniforms::modelMatrix, "modelMatrix");
			bindUniform(Uniforms::s_texture, "s_texture");
			bindUniform(Uniforms::textureFac, "textureFac");
			bindUniform(Uniforms::color, "color");
		}
	};
private:
	class ArrowMesh : public Mesh
	{
	public:
		virtual std::vector<glm::vec3> buildVertices() override;
	};
	class ScaleMesh : public Mesh
	{
	public:
		virtual std::vector<glm::vec3> buildVertices() override;
	};
	class ScaleCubeMesh : public Mesh
	{
	public:
		virtual std::vector<glm::vec3> buildVertices() override;
	};
	class RotateMesh : public Mesh
	{
	public:
		virtual std::vector<glm::vec3> buildVertices() override;
	};

	static inline SimpleShader* shader;
	static inline ArrowMesh arrowMesh;
	static inline RotateMesh rotateMesh;
	static inline ScaleMesh scaleMesh;
	static inline ScaleCubeMesh scaleCubeMesh;

	static inline glm::mat4 viewMatrix;
	static inline glm::mat4 projectionMatrix;
public:
	static void init();
	static void setMatrices(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);
	enum class Mode
	{
		Translate,
		Rotate,
		Scale,
		TranslateY,
	} mode = Mode::Translate;
	enum Axis
	{
		X = 1,
		Y = 2,
		Z = 4
	};
	static void setMode(Mode mode);
	
	bool axisClicked = false;
	bool axisReleased = false;
	bool axisDragged = false;
	int selectedAxis = 0;
	bool isMouseJustPressed = true;

	int selectedAxisIndex();

	void draw(const math::Ray& mouseRay, const glm::mat4& modelMatrix);
};