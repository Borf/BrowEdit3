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
			lightCount,
			light_position,
			light_color,
			light_type,
			light_falloff_style,
			light_range,
			light_intensity,
			light_cutoff,
			light_diffuseLighting,
			light_direction,
			light_width,
			light_falloff_0,
			light_falloff_1,
			light_falloff_2,
			light_falloff_3,
			light_falloff_4,
			light_falloff_5,
			light_falloff_6,
			light_falloff_7,
			light_falloff_8,
			light_falloff_9,
			light_falloff_count,
			hideOtherLights,
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
		bindUniform(Uniforms::lightCount, "lightCount");
		bindUniform(Uniforms::light_position, "lights[0].position");
		bindUniform(Uniforms::light_color, "lights[0].color");
		bindUniform(Uniforms::light_type, "lights[0].type");
		bindUniform(Uniforms::light_falloff_style, "lights[0].falloff_style");
		bindUniform(Uniforms::light_range, "lights[0].range");
		bindUniform(Uniforms::light_intensity, "lights[0].intensity");
		bindUniform(Uniforms::light_cutoff, "lights[0].cutoff");
		bindUniform(Uniforms::light_diffuseLighting, "lights[0].diffuseLighting");
		bindUniform(Uniforms::light_direction, "lights[0].direction");
		bindUniform(Uniforms::light_width, "lights[0].width");
		bindUniform(Uniforms::light_falloff_0, "lights[0].falloff[0]");
		bindUniform(Uniforms::light_falloff_1, "lights[0].falloff[1]");
		bindUniform(Uniforms::light_falloff_2, "lights[0].falloff[2]");
		bindUniform(Uniforms::light_falloff_3, "lights[0].falloff[3]");
		bindUniform(Uniforms::light_falloff_4, "lights[0].falloff[4]");
		bindUniform(Uniforms::light_falloff_5, "lights[0].falloff[5]");
		bindUniform(Uniforms::light_falloff_6, "lights[0].falloff[6]");
		bindUniform(Uniforms::light_falloff_7, "lights[0].falloff[7]");
		bindUniform(Uniforms::light_falloff_8, "lights[0].falloff[8]");
		bindUniform(Uniforms::light_falloff_9, "lights[0].falloff[9]");
		bindUniform(Uniforms::light_falloff_count, "lights[0].falloff_count");
		bindUniform(Uniforms::hideOtherLights, "hideOtherLights");
	}
};