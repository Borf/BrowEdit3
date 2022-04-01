#version 420

uniform sampler2D s_texture;
uniform vec4 color = vec4(1,1,1,1);
in vec2 texCoord;
out vec4 fragColor;



void main()
{
	vec4 outColor = texture2D(s_texture, texCoord);
	outColor *= color;
	if(outColor.a < 0.1)
		discard;
	fragColor = outColor;
}