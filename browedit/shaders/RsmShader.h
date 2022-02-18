#pragma once

#include <browedit/gl/Shader.h>

class RsmShader : public gl::Shader
{
public:
	RsmShader() : gl::Shader("data/shaders/rsm", Uniforms::End) { bindUniforms(); }
	struct Uniforms
	{
		enum
		{
			projectionMatrix,
			cameraMatrix,
			modelMatrix,
			modelMatrix2,
			s_texture,
			billboard,
			lightDiffuse,
			lightAmbient,
			lightIntensity,
			lightDirection,
			selection,
			shadeType,
			lightToggle,
			viewTextures,
			End
		};
	};
	void bindUniforms() override
	{
		bindUniform(Uniforms::projectionMatrix, "projectionMatrix");
		bindUniform(Uniforms::cameraMatrix, "cameraMatrix");
		bindUniform(Uniforms::s_texture, "s_texture");
		bindUniform(Uniforms::modelMatrix, "modelMatrix");
		bindUniform(Uniforms::modelMatrix2, "modelMatrix2");
		bindUniform(Uniforms::lightAmbient, "lightAmbient");
		bindUniform(Uniforms::lightDiffuse, "lightDiffuse");
		bindUniform(Uniforms::lightIntensity, "lightIntensity");
		bindUniform(Uniforms::lightDirection, "lightDirection");
		bindUniform(Uniforms::selection, "selection");
		bindUniform(Uniforms::shadeType, "shadeType");
		bindUniform(Uniforms::lightToggle, "lightToggle");
		bindUniform(Uniforms::viewTextures, "viewTextures");
	}
};