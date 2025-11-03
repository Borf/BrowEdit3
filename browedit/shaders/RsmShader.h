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
			//lightIntensity,
			lightDirection,
			selected,
			shadeType,
			discardAlphaValue,
			lightToggle,
			viewTextures,
			enableCullFace,
			fogEnabled,
			fogColor,
			fogNear,
			fogFar,
			//fogExp,
			textureAnimToggle,
			texMat,
			reverseCullFace,
			selectedColor,
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
		bindUniform(Uniforms::modelMatrix2, "modelMatrix2");
		bindUniform(Uniforms::lightAmbient, "lightAmbient");
		bindUniform(Uniforms::lightDiffuse, "lightDiffuse");
		//bindUniform(Uniforms::lightIntensity, "lightIntensity");
		bindUniform(Uniforms::lightDirection, "lightDirection");
		bindUniform(Uniforms::selected, "selected");
		bindUniform(Uniforms::shadeType, "shadeType");
		bindUniform(Uniforms::discardAlphaValue, "discardAlphaValue");
		bindUniform(Uniforms::lightToggle, "lightToggle");
		bindUniform(Uniforms::viewTextures, "viewTextures");
		bindUniform(Uniforms::enableCullFace, "enableCullFace");
		bindUniform(Uniforms::fogEnabled, "fogEnabled");
		bindUniform(Uniforms::fogColor, "fogColor");
		bindUniform(Uniforms::fogNear, "fogNear");
		bindUniform(Uniforms::fogFar, "fogFar");
		//bindUniform(Uniforms::fogExp, "fogExp");
		bindUniform(Uniforms::textureAnimToggle, "textureAnimToggle");
		bindUniform(Uniforms::texMat, "texMat");
		bindUniform(Uniforms::reverseCullFace, "reverseCullFace");
		bindUniform(Uniforms::selectedColor, "selectedColor");
		bindUniform(Uniforms::selection, "selection");
	}
};