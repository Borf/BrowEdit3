#pragma once

#include <browedit/gl/Shader.h>
class LightmapperShader : public gl::Shader
{
public:
	LightmapperShader(std::string header) : gl::Shader("data/shaders/lightmapper", Uniforms::End, header) { bindUniforms(); }
	struct Uniforms
	{
		enum
		{
			lightDirection,
			gndHeight,
			lightmapHeight,
			lightmapWidth,
			quality,
			shadows,
			additiveShadow,
			End
		};
	};
	void bindUniforms() override
	{
		bindUniform(Uniforms::gndHeight, "gndHeight");
		bindUniform(Uniforms::lightmapHeight, "lightmapHeight");
		bindUniform(Uniforms::lightmapWidth, "lightmapWidth");
		bindUniform(Uniforms::quality, "quality");
		bindUniform(Uniforms::shadows, "shadows");
		bindUniform(Uniforms::additiveShadow, "additiveShadow");
	}
};