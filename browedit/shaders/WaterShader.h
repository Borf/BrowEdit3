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
			time,
			waterHeight,
			amplitude,
			animSpeed,
			End
		};
	};
	void bindUniforms() override
	{
		bindUniform(Uniforms::ProjectionMatrix, "projectionMatrix");
		bindUniform(Uniforms::ModelMatrix, "modelMatrix");
		bindUniform(Uniforms::ViewMatrix, "viewMatrix");
		bindUniform(Uniforms::s_texture, "s_texture");
		bindUniform(Uniforms::time, "time");
		bindUniform(Uniforms::waterHeight, "waterHeight");
		bindUniform(Uniforms::amplitude, "amplitude");
		bindUniform(Uniforms::animSpeed, "animSpeed");
	}
};