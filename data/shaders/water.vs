#version 420

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texcoord;
//layout (location = 2) in vec3 a_normal;

uniform mat4 modelMatrix = mat4(1.0);
uniform mat4 viewMatrix = mat4(1.0);
uniform mat4 projectionMatrix = mat4(1.0);


uniform float waterHeight;
uniform float amplitude = 1;
uniform float waveSpeed = 1;
uniform float wavePitch = 50;
uniform float time;

//out vec3 normal;
out vec2 texCoord;

void main()
{
	texCoord = a_texcoord;

	float height = waterHeight;
	//height += amplitude*cos(radians((waveSpeed*16.6667*time)+(a_position.x-a_position.z)*.1*wavePitch));
	height += amplitude*cos(radians((waveSpeed*50*time)+(a_position.x-a_position.z)*.1*wavePitch));


	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vec3(a_position.x, height, a_position.z),1);
}