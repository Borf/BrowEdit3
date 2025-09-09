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

uniform vec3 lightDiffuse;
uniform vec3 lightAmbient;
uniform vec3 lightDirection;
uniform bool reverseCullFace;

out vec2 texCoord;
out vec3 normal;
out vec3 mult;
out float cull;

void main()
{
	mat3 normalMatrix = mat3(modelMatrix2 * modelMatrix); //TODO: move this to C++ code
	normal = normalMatrix * a_normal;
	normal = normalize(normal);
	cull = a_cull;

	texCoord = a_texture;
	vec4 billboarded = projectionMatrix * (cameraMatrix * modelMatrix) * vec4(0.0,0.0,0.0,1.0) +  modelMatrix2 * vec4(a_position.x, a_position.y,0.0,1.0);
	vec4 position = projectionMatrix * cameraMatrix * modelMatrix2 * modelMatrix * vec4(a_position,1.0);

	// Calculate shading on the vertex instead of fragment shader for better smoothing
	vec3 normal2 = normal;
	
	if (reverseCullFace)
		normal2 *= -1;
	
	float NL = clamp(dot(normal2, lightDirection), 0.0, 1.0);
	vec3 ambientFactor = (1.0 - lightAmbient) * lightAmbient;
	vec3 ambient = lightAmbient - ambientFactor + ambientFactor * lightDiffuse;
	vec3 diffuseFactor = (1.0 - lightDiffuse) * lightDiffuse;
	vec3 diffuse = lightDiffuse - diffuseFactor + diffuseFactor * lightAmbient;
	vec3 mult1 = min(NL * diffuse + ambient, 1.0);
	vec3 mult2 = min(max(lightDiffuse, lightAmbient) + (1.0 - max(lightDiffuse, lightAmbient)) * min(lightDiffuse, lightAmbient), 1.0);
	mult = min(mult1, mult2);
	
	gl_Position = mix(position, billboarded, billboard);
}