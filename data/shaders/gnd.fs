#version 420

uniform sampler2D s_texture;
uniform sampler2D s_lighting;

uniform vec3 lightDiffuse;
uniform vec3 lightAmbient;
uniform vec3 lightDirection;
//uniform float lightIntensity;
uniform float lightToggle = 1.0f;
uniform float colorToggle = 1.0f;
uniform float lightColorToggle = 1.0f;
uniform float shadowMapToggle = 1.0f;
uniform float viewTextures = 1.0f;

uniform bool fogEnabled;
uniform float fogNear = 0;
uniform float fogFar = 1;
//uniform float fogExp = 0.5;
uniform vec4 fogColor = vec4(1,1,1,1);

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
	
	float NL = clamp(dot(normalize(normal), vec3(1,-1,1)*lightDirection),0.0,1.0);
	vec3 ambientFactor = (1.0 - lightAmbient) * lightAmbient;
	vec3 ambient = lightAmbient - ambientFactor + ambientFactor * lightDiffuse;
	vec3 diffuseFactor = (1.0 - lightDiffuse) * lightDiffuse;
	vec3 diffuse = lightDiffuse - diffuseFactor + diffuseFactor * lightAmbient;
	vec3 mult1 = min(NL * diffuse + ambient, 1.0);
	// The formula quite literally changes when the combined value of lightAmbient + lightDiffuse is greater than 1.0
	vec3 mult2 = min(max(lightDiffuse, lightAmbient) + (1.0 - max(lightDiffuse, lightAmbient)) * min(lightDiffuse, lightAmbient), 1.0);
	texture.rgb *= min(mult1, mult2);
	texture.rgb *= max(color, colorToggle).rgb;
	texture.rgb *= max(texture2D(s_lighting, texCoord2).a, shadowMapToggle);
	texture.rgb += clamp(texture2D(s_lighting, texCoord2).rgb, 0.0, 1.0) * lightColorToggle;

	if(fogEnabled)
	{
		float depth = gl_FragCoord.z / gl_FragCoord.w;
		float fogAmount = smoothstep(fogNear, fogFar, depth);
		texture = mix(texture, fogColor, fogAmount);
	}	

	//gl_FragData[0] = texture;
	//gl_FragData[1] = vec4(0,0,0,0);
	fragColor = texture;
}