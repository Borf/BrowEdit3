#pragma once

#include <browedit/gl/Shader.h>
class OutlineShader : public gl::Shader
{
public:
	OutlineShader() : gl::Shader("data/shaders/outline", Uniforms::End) { bindUniforms(); }
	struct Uniforms
	{
		enum
		{
			s_texture,
			s_depth_fbo,
			s_depth_outline,
			iResolution,
			selectionOverlay,
			selectionOutline,
			selectionOccludedOverlay,
			selectionOccludedOutline,
			End
		};
	};
	void bindUniforms() override
	{
		bindUniform(Uniforms::s_texture, "s_texture");
		bindUniform(Uniforms::s_depth_fbo, "s_depth_fbo");
		bindUniform(Uniforms::s_depth_outline, "s_depth_outline");
		bindUniform(Uniforms::iResolution, "iResolution");
		bindUniform(Uniforms::selectionOverlay, "selectionOverlay");
		bindUniform(Uniforms::selectionOutline, "selectionOutline");
		bindUniform(Uniforms::selectionOccludedOverlay, "selectionOccludedOverlay");
		bindUniform(Uniforms::selectionOccludedOutline, "selectionOccludedOutline");
	}
};