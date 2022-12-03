#version 420
out vec4 fragColor;
in vec3 normal;
in vec2 texCoord;

uniform sampler2D s_texture;
uniform sampler2D s_textureNext;
uniform float frameTime;

uniform bool fogEnabled;
uniform float fogNear = 0;
uniform float fogFar = 1;
uniform float fogExp = 0.5;
uniform vec4 fogColor = vec4(1,1,1,1);


void main()
{
    vec4 color = texture2D(s_texture, texCoord); 
    vec4 color2 = texture2D(s_textureNext, texCoord); 
    color.a = 0.564;
    color2.a = 0.564;

    vec4 texture = mix(color, color2, frameTime);
    if(fogEnabled)
	{
		float depth = gl_FragCoord.z / gl_FragCoord.w;
		float fogAmount = smoothstep(fogNear, fogFar, depth);
		texture = mix(texture, fogColor, fogAmount);
	}
    fragColor = texture;

}