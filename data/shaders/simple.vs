#version 420

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texcoord;
layout (location = 2) in vec3 a_normal;

uniform mat4 modelMatrix = mat4(1.0);
uniform mat4 viewMatrix = mat4(1.0);
uniform mat4 projectionMatrix = mat4(1.0);

out vec3 normal;
out vec2 texCoord;

void main()
{
	mat3 normalMatrix = mat3(viewMatrix * modelMatrix); //TODO: move this to C++ code
	normalMatrix = transpose(inverse(normalMatrix));
	normal = normalMatrix * a_normal;
	texCoord = a_texcoord;

	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(a_position,1);
}