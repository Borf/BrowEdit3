#version 420

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texture;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in float a_cull;

uniform mat4 projectionMatrix;
uniform mat4 cameraMatrix;
uniform mat4 modelMatrix;
uniform mat4 modelMatrix2;
uniform float billboard = 0.0f;
uniform float selection;

out vec2 texCoord;
out vec3 normal;
out float cull;

void main()
{
	mat3 normalMatrix = mat3(modelMatrix2 * modelMatrix); //TODO: move this to C++ code
	normalMatrix = transpose(inverse(normalMatrix));
	normal = normalMatrix * a_normal;
	cull = a_cull;

	texCoord = a_texture;
	vec4 billboarded = projectionMatrix * (cameraMatrix * modelMatrix) * vec4(0.0,0.0,0.0,1.0) +  modelMatrix2 * vec4(a_position.x, a_position.y,0.0,1.0);
	vec4 position = projectionMatrix * cameraMatrix * modelMatrix2 * modelMatrix * vec4(a_position,1.0);

	gl_Position = mix(position, billboarded, billboard);
}