#version 420

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texcoord;
//layout (location = 2) in vec3 a_normal;

uniform mat4 modelMatrix = mat4(1.0);
uniform mat4 viewMatrix = mat4(1.0);
uniform mat4 projectionMatrix = mat4(1.0);


uniform float waterHeight;
uniform float time;

//out vec3 normal;
out vec2 texCoord;

void main()
{
	mat3 normalMatrix = mat3(viewMatrix * modelMatrix);
	normalMatrix = transpose(inverse(normalMatrix));
	texCoord = a_texcoord;
//	normal = normalMatrix * a_normal;

	float height = waterHeight;
	height += 1*cos(time*2+a_position.x*0.05);
	height += 1*cos(time*2+a_position.z*0.05);


	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vec3(a_position.x, height, a_position.z),1);
}