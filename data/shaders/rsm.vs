#version 420

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texture;
layout (location = 2) in vec3 a_normal;

uniform mat4 projectionMatrix;
uniform mat4 cameraMatrix;
uniform mat4 modelMatrix;
uniform mat4 modelMatrix2;
uniform float billboard = 0.0f;

out vec2 texCoord;
out vec3 normal;

void main()
{
	texCoord = a_texture;
	//vec4 billboarded = projectionMatrix * (cameraMatrix * modelMatrix) * vec4(0.0,0.0,0.0,1.0) +  modelMatrix2 * vec4(a_position.x, a_position.y,0.0,1.0);
	vec4 position = projectionMatrix * cameraMatrix * modelMatrix2 * modelMatrix * vec4(a_position,1.0);

	mat3 normalMatrix = mat3(modelMatrix2 * modelMatrix); //TODO: move this to C++ code
	normalMatrix = transpose(inverse(normalMatrix));

	normal = normalMatrix * a_normal;

	gl_Position = position;//mix(position, billboarded, billboard);
}