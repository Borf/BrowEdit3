#pragma once

#include <browedit/gl/Shader.h>
class WaterShader : public gl::Shader
{
public:
	WaterShader() : gl::Shader("data/shaders/water", Uniforms::End) { bindUniforms(); }
	struct Uniforms
	{
		enum
		{
			ProjectionMatrix,
			ViewMatrix,
			ModelMatrix,
			s_texture,
			s_textureNext,
			time,
			waterHeight,
			amplitude,
			waveSpeed,
			wavePitch,
			fogEnabled,
			fogColor,
			fogNear,
			fogFar,
			fogExp,
			End
		};
	};
	void bindUniforms() override
	{
		bindUniform(Uniforms::ProjectionMatrix, "projectionMatrix");
		bindUniform(Uniforms::ModelMatrix, "modelMatrix");
		bindUniform(Uniforms::ViewMatrix, "viewMatrix");
		bindUniform(Uniforms::s_texture, "s_texture");
		bindUniform(Uniforms::s_textureNext, "s_textureNext");
		bindUniform(Uniforms::time, "time");
		bindUniform(Uniforms::waterHeight, "waterHeight");
		bindUniform(Uniforms::amplitude, "amplitude");
		bindUniform(Uniforms::waveSpeed, "waveSpeed");
		bindUniform(Uniforms::wavePitch, "wavePitch");
		bindUniform(Uniforms::fogEnabled, "fogEnabled");
		bindUniform(Uniforms::fogColor, "fogColor");
		bindUniform(Uniforms::fogNear, "fogNear");
		bindUniform(Uniforms::fogFar, "fogFar");
		bindUniform(Uniforms::fogExp, "fogExp");
		
	}
};