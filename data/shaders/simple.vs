#version 420

layout (location = 0) in vec3 a_position;

uniform mat4 modelMatrix = mat4(1.0);
uniform mat4 viewMatrix = mat4(1.0);
uniform mat4 projectionMatrix = mat4(1.0);
uniform mat3 normalMatrix = mat3(1.0);

void main()
{
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(a_position,1);
}