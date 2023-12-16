#pragma once

#include <browedit/gl/Shader.h>

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
			lightMin,
			color,
			colorMult,
			lightDirection,
			shadeType,
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
		bindUniform(Uniforms::lightMin, "lightMin");
		bindUniform(Uniforms::color, "color");
		bindUniform(Uniforms::colorMult, "colorMult");
		bindUniform(Uniforms::lightDirection, "lightDirection");
		bindUniform(Uniforms::shadeType, "shadeType");
	}
};
