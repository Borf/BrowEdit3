#version 420

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_texture;
layout (location = 2) in float a_alpha;

uniform mat4 projectionMatrix;
uniform mat4 cameraMatrix;
uniform mat4 modelMatrix;
uniform bool billboard_off;

out vec2 texCoord;
out float alpha;

void main()
{
	texCoord = a_texture;
	alpha = a_alpha;
	
	if (billboard_off) {
		vec4 billboarded = projectionMatrix * (cameraMatrix * modelMatrix) * vec4(a_position,1.0);
		gl_Position = billboarded;
	}
	else {
		vec4 billboarded = projectionMatrix * (cameraMatrix * modelMatrix) * vec4(0.0,0.0,0.0,1.0);
		billboarded.xy += (projectionMatrix * vec4(a_position.x, a_position.y,0.0,1.0)).xy;
		gl_Position = billboarded;
	}
}