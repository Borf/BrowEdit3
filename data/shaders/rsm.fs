uniform sampler2D s_texture;
uniform vec4 highlightColor;

uniform vec3 lightDiffuse;
uniform vec3 lightAmbient;
uniform vec3 lightDirection;
uniform float lightIntensity;

uniform int shadeType;
uniform float selection;
uniform bool lightToggle;
uniform bool viewTextures;

uniform bool fogEnabled;
uniform float fogNear = 0;
uniform float fogFar = 1;
uniform float fogExp = 0.5;
uniform vec4 fogColor = vec4(1,1,1,1);

varying vec2 texCoord;
varying vec3 normal;




void main()
{
	vec4 color = texture2D(s_texture, texCoord);
	if(!viewTextures)
		color.rgb = vec3(1,1,1);
	if(color.a < 0.1)
		discard;

	if(shadeType == 1 || shadeType == 2 && lightToggle)
		color.rgb *= lightIntensity * clamp(dot(normalize(normal), lightDirection),0.0,1.0) * lightDiffuse + lightAmbient;

	if(shadeType == 4 && lightToggle) // only for editor
		color.rgb *= lightDiffuse;

	if(fogEnabled)
	{
		float depth = gl_FragCoord.z / gl_FragCoord.w;
		float fogAmount = smoothstep(fogNear, fogFar, depth);
		color = mix(color, fogColor, fogAmount);
	}	

	gl_FragData[0] = mix(color, vec4(1,0,0,1), min(1.0,selection));
	//gl_FragData[1] = highlightColor;
}