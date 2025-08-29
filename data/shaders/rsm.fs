uniform sampler2D s_texture;

uniform vec3 lightDiffuse;
uniform vec3 lightAmbient;
uniform vec3 lightDirection;
//uniform float lightIntensity;

uniform int shadeType;
uniform float discardAlphaValue;
uniform float selection;
uniform bool lightToggle;
uniform bool viewTextures;
uniform bool enableCullFace;
uniform bool reverseCullFace;
in float cull;

uniform bool fogEnabled;
uniform float fogNear;
uniform float fogFar;
//uniform float fogExp;
uniform vec4 fogColor;

varying vec2 texCoord;
varying vec3 normal;
varying vec3 mult;

//texture animation
uniform bool textureAnimToggle;
uniform mat4 texMat;

void main()
{
	if (enableCullFace && cull <= 0) {
		if (cull >= 0 && !gl_FrontFacing)
			discard;
		if (cull < 0 && gl_FrontFacing)
			discard;
	}
	
	vec2 texCoord2 = texCoord;
	
	if (textureAnimToggle) {
		texCoord2 = vec2(texMat * vec4(texCoord2.x, texCoord2.y, 0, 1));
	}
	
	vec4 color = texture2D(s_texture, texCoord2);
	if (color.a < discardAlphaValue)
		discard;
	if(!viewTextures)
		color.rgb = vec3(1,1,1);
	if (lightToggle) {
		if (shadeType == 4) { // only for editor
			color.rgb *= lightDiffuse;
		}
		else if (shadeType == 0) {
			color.rgb *= min(max(lightDiffuse, lightAmbient) + (1.0 - max(lightDiffuse, lightAmbient)) * min(lightDiffuse, lightAmbient), 1.0);
		}
		else {
			color.rgb *= mult;
		}
	}
	
	if(fogEnabled)
	{
		float depth = gl_FragCoord.z / gl_FragCoord.w;
		float fogAmount = smoothstep(fogNear, fogFar, depth);
		color = mix(color, fogColor, fogAmount);
	}	

	gl_FragData[0] = mix(color, vec4(1,0,0,1), min(1.0,selection));
}