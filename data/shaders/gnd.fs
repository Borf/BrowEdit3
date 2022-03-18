#version 420

uniform sampler2D s_texture;
uniform sampler2D s_lighting;

uniform vec3 lightDiffuse;
uniform vec3 lightAmbient;
uniform vec3 lightDirection;
uniform float lightIntensity;
uniform float lightToggle = 1.0f;
uniform float colorToggle = 1.0f;
uniform float lightColorToggle = 1.0f;
uniform float shadowMapToggle = 1.0f;
uniform float viewTextures = 1.0f;

in vec2 texCoord;
in vec2 texCoord2;
in vec3 normal;
in vec4 color;

out vec4 fragColor;
//out vec4 fragSelection;

void main()
{
	vec4 texture = vec4(1,1,1,1);

	vec4 texColor = texture2D(s_texture, texCoord);
	texture = mix(vec4(1,1,1,texColor.a), texColor, viewTextures);
	if(texture.a < 0.1)
		discard;

	texture.rgb *= max(color, colorToggle).rgb;
	texture.rgb *= max(texture2D(s_lighting, texCoord2).a, shadowMapToggle);


	texture.rgb *= max((max(0.0, dot(normal, vec3(-1,-1,1)*lightDirection)) * lightDiffuse + lightIntensity * lightAmbient), lightToggle);
	texture += clamp(vec4(texture2D(s_lighting, texCoord2).rgb,1.0), 0.0, 1.0) * lightColorToggle;


	//gl_FragData[0] = texture;
	//gl_FragData[1] = vec4(0,0,0,0);
	fragColor = texture;
}