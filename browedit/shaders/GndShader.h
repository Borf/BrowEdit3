#pragma once

#include <browedit/gl/Shader.h>
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
			lightAmbient,
			lightDiffuse,
			//lightIntensity,
			lightDirection,
			shadowMapToggle,
			lightToggle,
			colorToggle,
			lightColorToggle,
			viewTextures,
			fogEnabled,
			fogColor,
			fogNear,
			fogFar,
			//fogExp,
			End
		};
	};
	void bindUniforms() override
	{
		bindUniform(Uniforms::ProjectionMatrix, "projectionMatrix");
		bindUniform(Uniforms::ModelViewMatrix, "modelViewMatrix");
		bindUniform(Uniforms::s_texture, "s_texture");
		bindUniform(Uniforms::s_lighting, "s_lighting");
		bindUniform(Uniforms::lightAmbient, "lightAmbient");
		bindUniform(Uniforms::lightDiffuse, "lightDiffuse");
		//bindUniform(Uniforms::lightIntensity, "lightIntensity");
		bindUniform(Uniforms::lightDirection, "lightDirection");
		bindUniform(Uniforms::shadowMapToggle, "shadowMapToggle");
		bindUniform(Uniforms::lightToggle, "lightToggle");
		bindUniform(Uniforms::colorToggle, "colorToggle");
		bindUniform(Uniforms::lightColorToggle, "lightColorToggle");
		bindUniform(Uniforms::viewTextures, "viewTextures");
		bindUniform(Uniforms::fogEnabled, "fogEnabled");
		bindUniform(Uniforms::fogColor, "fogColor");
		bindUniform(Uniforms::fogNear, "fogNear");
		bindUniform(Uniforms::fogFar, "fogFar");
		//bindUniform(Uniforms::fogExp, "fogExp");
	}
};