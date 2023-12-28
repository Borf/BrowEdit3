#version 420
out vec4 fragColor;
in vec3 normal;
in vec2 texCoord;

uniform sampler2D s_texture;

uniform bool fogEnabled;
uniform float fogNear = 0;
uniform float fogFar = 1;
uniform float fogExp = 0.5;
uniform vec4 fogColor = vec4(1,1,1,1);


void main()
{
    vec4 color = texture2D(s_texture, texCoord);
    color.a = 0.564;

    if(fogEnabled)
	{
		float depth = gl_FragCoord.z / gl_FragCoord.w;
		float fogAmount = smoothstep(fogNear, fogFar, depth);
		color = mix(color, fogColor, fogAmount);
	}
    fragColor = color;

}